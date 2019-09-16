/*******************************************************************************
**          File: looper.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-08-25 Sun 10:36 AM
**   Description: a loop that schedules and runs queued tasks in a thread
*******************************************************************************/
#ifndef LOOPER_H_
#define LOOPER_H_
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <deque>
#include <chrono>
#include <pthread.h>

#include "spin_lock.hpp"

#ifdef __ANDROID__
#include <sys/prctl.h>
#endif

#if __cplusplus == 201103L || (defined(_MSC_VER) && _MSC_VER == 1900)
namespace std {
  template<class T> struct _Unique_if {
    typedef unique_ptr<T> _Single_object;
  };

  template<class T> struct _Unique_if<T[]> {
    typedef unique_ptr<T[]> _Unknown_bound;
  };

  template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
  };

  template<class T, class... Args>
    typename _Unique_if<T>::_Single_object
    make_unique(Args&&... args) {
      return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

  template<class T>
    typename _Unique_if<T>::_Unknown_bound
    make_unique(size_t n) {
      typedef typename remove_extent<T>::type U;
      return unique_ptr<T>(new U[n]());
    }

  template<class T, class... Args>
    typename _Unique_if<T>::_Known_bound
    make_unique(Args&&...) = delete;
}
#endif

namespace {
  class Task final {
    public:
      Task(void *marker, std::function<void()> &&call) :
        marker(marker), call(call) {}

      void *marker;
      std::function<void()> call;
  };

  class TimedTask final {
    public:
      TimedTask(
        void *marker,
        int identity,
        int64_t triggerTimeUs,
        int64_t intervalUs,
        std::function<void()> &&call) :
        marker(marker),
        identity(identity),
        triggerTimeUs(triggerTimeUs),
        intervalUs(intervalUs),
        call(call) {}

      void *marker;
      int identity; // a number to identify this task
      int64_t triggerTimeUs;
      int64_t intervalUs; // zero if no repeat
      std::function<void()> call;
  };
}

namespace nul {

  class TaskQueue;
  class Looper final : public std::enable_shared_from_this<Looper> {
    friend class TaskQueue;
    private:
      Looper(const std::string &name = "") : name_(name) { }

    public:
      ~Looper() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (t_) {
          lock.unlock();
          stop(true);
          t_->join();
        }
      }

      static std::shared_ptr<Looper> create(const std::string &name = "") {
        return std::shared_ptr<Looper>(new Looper(name));
      }

      static std::shared_ptr<Looper> getCurrent() {
        auto looper = pthread_getspecific(getThreadLocalLooperKey());
        if (looper) {
          return reinterpret_cast<Looper *>(looper)->shared_from_this();
        }
        return nullptr;
      }

      void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!t_) {
          running_ = true;
          t_ = std::make_unique<std::thread>(&Looper::run, this);
        }
      }

      void stop(bool gracefulStop = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_ && gracefulStop) {
          gracefulStopping_ = true;
        } else {
          running_ = false;
        }
        cond_.notify_one();
      }

      std::string getName() const {
        return name_;
      }

      bool isRunning() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return running_;
      }

    private:
      bool postTask(std::unique_ptr<Task> task) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || gracefulStopping_) {
          return false;
        }
        q_.push_back(std::move(task));

        cond_.notify_one();
        return true;
      }

      bool postTimedTask(std::unique_ptr<TimedTask> timedTask) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || gracefulStopping_) {
          return false;
        }

        auto inserted = false;
        auto it = delayedQ_.begin();
        while (it != delayedQ_.end()) {
          if (timedTask->triggerTimeUs < (*it)->triggerTimeUs) {
            delayedQ_.insert(it, std::move(timedTask));
            inserted = true;
            break;
          }
          ++it;
        }
        if (!inserted) {
          delayedQ_.push_back(std::move(timedTask));
        }

        // call notify_one only when task is inserted at the head of the queue
        if (delayedQ_.size() == 1 || it == (delayedQ_.begin() + 1)) {
          cond_.notify_one();
        }
        return true;
      }

      void removePendingTasks(void *marker, int identity) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = delayedQ_.begin();
        while (it != delayedQ_.end()) {
          auto &task = *it;
          if (task->marker == marker && task->identity == identity) {
            it = delayedQ_.erase(it);
          } else {
            ++it;
          }
        }
      }

      void removeAllPendingTasks(void *marker) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it1 = q_.begin();
        while (it1 != q_.end()) {
          remove(q_, it1, marker);
        }

        auto it2 = delayedQ_.begin();
        while (it2 != delayedQ_.end()) {
          remove(delayedQ_, it2, marker);
        }
      }

    private:
      void run() {
        pthread_setspecific(getThreadLocalLooperKey(), this);

        if (!name_.empty()) {
#ifdef __ANDROID__
          prctl(PR_SET_NAME, (unsigned long)name_.c_str(), 0, 0, 0);
#elif __APPLE__
          pthread_setname_np(name_.c_str());
#endif
        }

        using namespace std::chrono;

        while (running_) {
          auto lock = std::unique_lock<std::mutex>(mutex_);
          if (!q_.empty()) {
            auto task = std::move(q_.front());
            q_.pop_front();

            lock.unlock();
            task->call();
            continue;
          }

          if (!delayedQ_.empty()) {
            auto now = duration_cast<microseconds>(
              system_clock::now().time_since_epoch()).count();
            auto &timedTaskRef = delayedQ_.front();
            if (now >= timedTaskRef->triggerTimeUs) {
              auto timedTask = std::move(delayedQ_.front());
              delayedQ_.pop_front();
              lock.unlock();

              timedTask->call();

              if (timedTask->intervalUs > 0) {
                timedTask->triggerTimeUs += timedTask->intervalUs;
                postTimedTask(std::move(timedTask));
              }
              continue;
            }
          }

          if (running_) {
            // if in gracefulStopping_ state, delayed tasks will be ignored
            if (gracefulStopping_) {
              running_ = false;
              break;
            }

            if (!delayedQ_.empty()) {
              auto triggerTimeUs = delayedQ_.front()->triggerTimeUs;
              auto delay = triggerTimeUs - duration_cast<microseconds>(
                system_clock::now().time_since_epoch()).count();
              if (delay <= 0) {
                continue;
              }
              cond_.wait_for(lock, microseconds(delay));

            } else {
              cond_.wait(lock);
            }
          }
        }
      }

      template <typename Q, typename It>
      void remove(Q &&q, It &&it, void *marker) {
        if ((*it)->marker == marker) {
          it = q.erase(it);
        } else {
          ++it;
        }
      }

      static pthread_key_t getThreadLocalLooperKey() {
        static pthread_key_t looperKey;
        static std::once_flag flag;
        std::call_once(flag, [] {
          pthread_key_create(&looperKey, nullptr);
        });
        return looperKey;
      }

    private:
      std::deque<std::unique_ptr<Task>> q_;             // guarded by mutex_
      std::deque<std::unique_ptr<TimedTask>> delayedQ_; // guarded by mutex_
      std::unique_ptr<std::thread> t_{nullptr};
      std::condition_variable cond_;
      mutable std::mutex mutex_;

      std::string name_;
      bool running_{false};           // guarded by mutex_
      bool gracefulStopping_{false};  // guarded by mutex_
  };

  class TaskQueue final {
    public:
      TaskQueue(const std::shared_ptr<Looper> &looper) : looper_(looper) {
        assert(!!looper);
      }

      template <typename Callable, typename ...Args>
      bool post(Callable &&call, Args &&...args) {
        auto lock = SpinLock(busyFlag_);
        return !detached_ && looper_->postTask(std::make_unique<Task>(
            this, std::bind(
              std::forward<Callable>(call), std::forward<Args>(args)...)));
      }

      template <typename Callable, typename ...Args>
      bool postDelayed(int64_t delayUs, Callable &&call, Args &&...args) {
        return postAtIntervalInternal(
          0, delayUs, 0,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      template <typename Callable, typename ...Args>
      bool postDelayedWithId(
        int identity, int64_t delayUs, Callable &&call, Args &&...args) {
        return postAtIntervalInternal(
          identity, delayUs, 0,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      template <typename Callable, typename ...Args>
      bool postAtInterval(
        int64_t delayUs, int64_t intervalUs, Callable &&call, Args &&...args) {
        return postAtIntervalInternal(
          0, delayUs, intervalUs,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      template <typename Callable, typename ...Args>
      bool postAtIntervalWithId(
        int identity, int64_t delayUs, int64_t intervalUs,
        Callable &&call, Args &&...args) {
        return postAtIntervalInternal(
          identity, delayUs, intervalUs,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      void remove(int identity) {
        // no lock is needed here because looper_ itself is thread-safe
        if (!detached_) {
          looper_->removePendingTasks(this, identity);
        }
      }

      void removeAllPendingTasks() {
        // no lock is needed here because looper_ itself is thread-safe
        if (!detached_) {
          looper_->removeAllPendingTasks(this);
        }
      }

      void detachFromLooper() {
        auto lock = SpinLock(busyFlag_);
        detached_ = true;
        looper_->removeAllPendingTasks(this);
      }

      std::string getName() const {
        return !detached_ ? looper_->getName() : "";
      }

      bool isRunning() const {
        return !detached_ && looper_->isRunning();
      }

    private:
      template <typename Callable, typename ...Args>
      bool postAtIntervalInternal(
        int identity, int64_t delayUs, int64_t intervalUs,
        Callable &&call, Args &&...args) {

        if (delayUs < 0) {
          delayUs = 0;
        }

        using namespace std::chrono;
        auto triggerTimeUs = duration_cast<microseconds>(
          system_clock::now().time_since_epoch()).count() + delayUs;

        auto timedTask = std::make_unique<TimedTask>(
          this, identity, triggerTimeUs, intervalUs,
          std::bind(std::forward<Callable>(call), std::forward<Args>(args)...)
        );

        auto lock = SpinLock(busyFlag_);
        return !detached_ && looper_->postTimedTask(std::move(timedTask));
      }

    private:
      std::shared_ptr<Looper> looper_;
      bool detached_{false};
      std::atomic_flag busyFlag_ = ATOMIC_FLAG_INIT;
  };

} /* end of namespace: nul */

#endif /* end of include guard: LOOPER_H_ */
