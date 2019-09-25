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
#include "cpp11_compat.hpp"

#ifdef __ANDROID__
#include <sys/prctl.h>
#endif

namespace {
  class Task {
    public:
      Task(void *marker, int identity, std::function<void()> &&call) :
        marker(marker), identity(identity), call(std::move(call)) {}

      void *marker;
      int identity; // a number to name this task, zero if unamed
      std::function<void()> call;
  };

  class TimedTask : public Task {
    public:
      TimedTask(
        void *marker,
        int identity,
        int64_t triggerTimeUs,
        int64_t intervalUs,
        std::function<void()> &&call) :
        Task(marker, identity, std::move(call)),
        triggerTimeUs(triggerTimeUs),
        intervalUs(intervalUs) {}

      int64_t triggerTimeUs;
      int64_t intervalUs; // zero if no repeat
      bool isRemoved{false};
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
          stop();
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

      void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
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
        if (!running_) {
          return false;
        }
        q_.push_back(std::move(task));

        cond_.notify_one();
        return true;
      }

      bool postTimedTask(std::unique_ptr<TimedTask> timedTask) {
        std::lock_guard<std::mutex> lock(mutex_);
        return postTimedTaskLocked(std::move(timedTask));
      }

      // the main lock must be held when calling this function
      bool postTimedTaskLocked(std::unique_ptr<TimedTask> timedTask) {
        if (!running_) {
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

      using RemoveTaskComparator = std::function<bool(const Task &task)>;

      void removePendingTasks(void *marker, int identity) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto comp = [marker, identity](const Task &task){
          return marker == task.marker && identity == task.identity;
        };
        doRemoveTasks(q_, comp);
        doRemoveTasks(delayedQ_, comp);

        if (activeRepeatedTimedTask_ &&
            activeRepeatedTimedTask_->marker == marker &&
            activeRepeatedTimedTask_->identity == identity) {
          activeRepeatedTimedTask_->isRemoved = true;
        }
      }

      void removeAllPendingTasks(void *marker) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto comp = [marker](const Task &task){
          return marker == task.marker;
        };
        doRemoveTasks(q_, comp);
        doRemoveTasks(delayedQ_, comp);

        if (activeRepeatedTimedTask_ &&
            activeRepeatedTimedTask_->marker == marker) {
          activeRepeatedTimedTask_->isRemoved = true;
        }
      }

      // remove all tasks that do not have identities
      void removeAllUnamedPendingTasks(void *marker) {
        removePendingTasks(marker, 0);
      }

      void removeAllNonRepeatedTasks(void *marker) {
        std::lock_guard<std::mutex> lock(mutex_);

        doRemoveTasks(q_, [marker](const Task &task){
          return marker == task.marker;
        });
        doRemoveTasks(delayedQ_, [marker](const Task &task){
          return marker == task.marker &&
            static_cast<const TimedTask &>(task).intervalUs == 0;
        });

        if (activeRepeatedTimedTask_ &&
            activeRepeatedTimedTask_->marker == marker &&
            activeRepeatedTimedTask_->intervalUs == 0) {
          activeRepeatedTimedTask_->isRemoved = true;
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

          if (delayedQ_.empty()) {
            cond_.wait(lock);
            continue;
          }

          auto now = duration_cast<microseconds>(
            high_resolution_clock::now().time_since_epoch()).count();
          auto delay = delayedQ_.front()->triggerTimeUs - now;

          if (delay > 0) {
            cond_.wait_for(lock, microseconds(delay));
            continue;
          }

          auto timedTask = std::move(delayedQ_.front());
          delayedQ_.pop_front();
          if (timedTask->intervalUs > 0) {
            activeRepeatedTimedTask_ = timedTask.get();
          }
          lock.unlock();

          timedTask->call();

          if (timedTask->intervalUs > 0) {
            lock = std::unique_lock<std::mutex>(mutex_);
            activeRepeatedTimedTask_ = nullptr;
            if (!timedTask->isRemoved) {
              timedTask->triggerTimeUs += timedTask->intervalUs;
              postTimedTaskLocked(std::move(timedTask));
            }
          }
        }
      }

      template <typename Q>
      void doRemoveTasks(Q &&q, const RemoveTaskComparator &comp) {
        auto it = q.begin();
        while (it != q.end()) {
          if (comp(**it)) {
            it = q.erase(it);
          } else {
            ++it;
          }
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

      TimedTask *activeRepeatedTimedTask_{nullptr}; // guarded by mutex_
  };

  class TaskQueue final {
    public:
      TaskQueue(const std::shared_ptr<Looper> &looper) : looper_(looper) {
        assert(!!looper);
      }

      template <typename Callable, typename ...Args>
      bool post(Callable &&call, Args &&...args) {
        return post(0, std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      // identity=0 means no name for this task, which will be deleted once
      // removeAllUnamedPendingTasks() is called
      template <typename Callable, typename ...Args>
      bool post(int identity, Callable &&call, Args &&...args) {
        auto lock = SpinLock(busyFlag_);
        return !detached_ && looper_->postTask(std::make_unique<Task>(
            this, identity, std::bind(
              std::forward<Callable>(call), std::forward<Args>(args)...)));
      }

      template <typename Callable, typename ...Args>
      bool postDelayed(int64_t delayUs, Callable &&call, Args &&...args) {
        return postRepeatedInternal(
          0, delayUs, 0,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      // identity=0 means no name for this task, which will be deleted once
      // removeAllUnamedPendingTasks() is called
      template <typename Callable, typename ...Args>
      bool postDelayedWithId(
        int identity, int64_t delayUs, Callable &&call, Args &&...args) {
        return postRepeatedInternal(
          identity, delayUs, 0,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      template <typename Callable, typename ...Args>
      bool postRepeated(
        int64_t delayUs, int64_t intervalUs, Callable &&call, Args &&...args) {
        return postRepeatedInternal(
          0, delayUs, intervalUs,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      // identity=0 means no name for this task, which will be deleted once
      // removeAllUnamedPendingTasks() is called
      template <typename Callable, typename ...Args>
      bool postRepeatedWithId(
        int identity, int64_t delayUs, int64_t intervalUs,
        Callable &&call, Args &&...args) {
        return postRepeatedInternal(
          identity, delayUs, intervalUs,
          std::forward<Callable>(call), std::forward<Args>(args)...);
      }

      void removePendingTasks(int identity) {
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

      // remove all tasks that do not have identities
      void removeAllUnamedPendingTasks() {
        // no lock is needed here because looper_ itself is thread-safe
        if (!detached_) {
          looper_->removeAllUnamedPendingTasks(this);
        }
      }

      void removeAllNonRepeatedTasks() {
        if (!detached_) {
          looper_->removeAllNonRepeatedTasks(this);
        }
      }

      void detachFromLooper() {
        auto lock = SpinLock(busyFlag_);
        if (!detached_) {
          detached_ = true;
          looper_->removeAllPendingTasks(this);
        }
      }

      template <typename Callable, typename ...Args>
      void detachFromLooper(Callable &&finalizer, Args &&...args) {
        auto lock = SpinLock(busyFlag_);
        if (detached_) {
          return;
        }

        detached_ = true;
        // remove all pending tasks before posting the last task
        looper_->removeAllPendingTasks(this);

        // give the caller a chance to run the last task, the caller can use
        // this task to keep a reference (probably a shared_ptr from 
        // shared_from_this()) to the caller itself to avoid the case that
        // pending tasks access the caller object's raw pointer while the
        // caller was already deallocated
        looper_->postTask(std::make_unique<Task>(
            this, 0, std::bind(
              std::forward<Callable>(finalizer), std::forward<Args>(args)...)));
      }

      std::string getName() const {
        return !detached_ ? looper_->getName() : "";
      }

      bool isRunning() const {
        return !detached_ && looper_->isRunning();
      }

    private:
      template <typename Callable, typename ...Args>
      bool postRepeatedInternal(
        int identity, int64_t delayUs, int64_t intervalUs,
        Callable &&call, Args &&...args) {

        auto lock = SpinLock(busyFlag_);
        if (detached_) {
          return false;
        }

        if (delayUs < 0) {
          delayUs = 0;
        }

        using namespace std::chrono;
        auto triggerTimeUs = duration_cast<microseconds>(
          high_resolution_clock::now().time_since_epoch()).count() + delayUs;

        auto timedTask = std::make_unique<TimedTask>(
          this, identity, triggerTimeUs, intervalUs,
          std::bind(std::forward<Callable>(call), std::forward<Args>(args)...)
        );

        return looper_->postTimedTask(std::move(timedTask));
      }

    private:
      std::shared_ptr<Looper> looper_;
      bool detached_{false};
      std::atomic_flag busyFlag_ = ATOMIC_FLAG_INIT;
  };

} /* end of namespace: nul */

#endif /* end of include guard: LOOPER_H_ */
