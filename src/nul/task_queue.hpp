/*******************************************************************************
**          File: task_queue.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-08-25 Sun 10:36 AM
**   Description: a class that schedules and runs queued tasks in a dedicated
**                thread or in std thread pool using std::async
*******************************************************************************/
#ifndef TASK_QUEUE_H_
#define TASK_QUEUE_H_
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <deque>
#include <chrono>
#include <future>

#if __cplusplus == 201103L || (defined(_MSC_VER) && _MSC_VER == 1900)
namespace std {
  template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique(Args&& ...args) {
      return std::unique_ptr<T>(new T( std::forward<Args>(args)...));
    }
}
#endif

namespace nul {

  class TimedTask final {
    public:
      TimedTask(
        const std::string &name,
        int64_t triggerTimeMs,
        int64_t intervalMs,
        std::function<void()> task) :
        name(name),
        triggerTimeMs(triggerTimeMs),
        intervalMs(intervalMs),
        task(task) {}

      std::string name;
      int64_t triggerTimeMs;
      int64_t intervalMs; // zero if no repeat
      std::function<void()> task;
  };

  class TaskQueue final {
    public:
      TaskQueue(bool useStdAsync = false) : useStdAsync_(useStdAsync) {}
      TaskQueue(const TaskQueue &) = delete;
      TaskQueue &operator=(const TaskQueue &) = delete;

      ~TaskQueue() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (t_) {
          lock.unlock();
          stop(true);
          t_->join();
        }
      }

      template <typename Callbale, typename ...Args>
      bool post(Callbale task, Args ...args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || gracefulStopping_) {
          return false;
        }
        q_.push(std::bind(task, std::forward<Args>(args)...));
        cond_.notify_one();
        return true;
      }

      template <typename Callbale, typename ...Args,
               typename R = typename std::result_of<Callbale(Args...)>::type>
      R postSync(Callbale task, Args ...args) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!running_ || gracefulStopping_) {
          return task(std::forward<Args>(args)...);
        }

        auto p = std::promise<R>();
        auto f = p.get_future();
        q_.push([&]{ p.set_value(task(std::forward<Args>(args)...)); });
        lock.unlock();
        cond_.notify_one();

        return f.get();
      }

      template <typename Callbale, typename ...Args>
      bool postDelayed(int64_t delayMs, Callbale task, Args ...args) {
        return postAtIntervalInternal(
          "", delayMs, 0, task, std::forward<Args>(args)...);
      }

      template <typename Callbale, typename ...Args>
      bool postDelayed(
        const std::string &name, int64_t delayMs, Callbale task, Args ...args) {
        return postAtIntervalInternal(
          name, delayMs, 0, task, std::forward<Args>(args)...);
      }

      template <typename Callbale, typename ...Args>
      bool postAtInterval(
        int64_t delayMs, int64_t intervalMs, Callbale task, Args ...args) {
        return postAtIntervalInternal(
          "", delayMs, intervalMs, task, std::forward<Args>(args)...);
      }

      template <typename Callbale, typename ...Args>
      bool postAtInterval(
        const std::string &name, int64_t delayMs, int64_t intervalMs,
        Callbale task, Args ...args) {
        return postAtIntervalInternal(
          name, delayMs, intervalMs, task, std::forward<Args>(args)...);
      }

      void remove(const std::string &name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = delayedQ_.begin();
        while (it != delayedQ_.end()) {
          if (name == (*it)->name) {
            it = delayedQ_.erase(it);
          } else {
            ++it;
          }
        }

        cond_.notify_one();
      }

      void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!t_) {
          running_ = true;
          t_ = std::make_unique<std::thread>(&TaskQueue::run, this);
        }
      }

      void stop(bool gracefulStop = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_ && gracefulStop) {
          gracefulStopping_ = true;
        } else {
          running_ = false;
        }
        cond_.notify_all();
      }

    private:
      void run() {
        using namespace std::chrono;

        while (running_) {
          auto lock = std::unique_lock<std::mutex>(mutex_);
          if (!q_.empty()) {
            auto task = std::move(q_.front());
            q_.pop();

            lock.unlock();
            if (useStdAsync_) {
              std::async(std::launch::async, std::cref(task));
            } else {
              task();
            }
          }

          if (!lock.owns_lock()) {
            lock = std::unique_lock<std::mutex>(mutex_);
          }
          if (!delayedQ_.empty()) {
            auto now = duration_cast<milliseconds>(
              system_clock::now().time_since_epoch()).count();
            auto &timedTaskRef = delayedQ_.front();
            if (now >= timedTaskRef->triggerTimeMs) {
              auto timedTask = std::move(delayedQ_.front());
              delayedQ_.pop_front();
              lock.unlock();

              if (useStdAsync_) {
                std::async(std::launch::async, std::cref(timedTask->task));
              } else {
                timedTask->task();
              }

              if (timedTask->intervalMs > 0) {
                timedTask->triggerTimeMs += timedTask->intervalMs;
                addTimedTask(std::move(timedTask));
              }
            }
          }

          if (!lock.owns_lock()) {
            lock = std::unique_lock<std::mutex>(mutex_);
          }
          if (running_ && q_.empty()) {
            // if in gracefulStopping_ state, delayed tasks will be ignored
            if (gracefulStopping_) {
              running_ = false;
              break;
            }

            if (!delayedQ_.empty()) {
              auto triggerTimeMs = delayedQ_.front()->triggerTimeMs;
              auto delay = triggerTimeMs - duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()).count();
              cond_.wait_for(lock, milliseconds(delay > 0 ? delay : 0));

            } else {
              cond_.wait(lock);
            }
          }
        }
      }

      template <typename Callbale, typename ...Args>
      bool postAtIntervalInternal(
        const std::string &name, int64_t delayMs,
        int64_t intervalMs, Callbale task, Args ...args) {

        if (delayMs < 0) {
          delayMs = 0;
        }

        using namespace std::chrono;
        auto triggerTimeMs = duration_cast<milliseconds>(
          system_clock::now().time_since_epoch()).count() + delayMs;

        auto timedTask = std::make_unique<TimedTask>(
          name, triggerTimeMs, intervalMs,
          std::bind(task, std::forward<Args>(args)...)
        );

        return addTimedTask(std::move(timedTask));
      }

      bool addTimedTask(std::unique_ptr<TimedTask> timedTask) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || gracefulStopping_) {
          return false;
        }

        auto inserted = false;
        auto it = delayedQ_.begin();
        while (it != delayedQ_.end()) {
          if (timedTask->triggerTimeMs < (*it)->triggerTimeMs) {
            delayedQ_.insert(it, std::move(timedTask));
            inserted = true;
            break;
          }
          ++it;
        }
        if (!inserted) {
          delayedQ_.push_back(std::move(timedTask));
        }

        cond_.notify_one();
        return true;
      }
    
    private:
      std::queue<std::function<void()>> q_;
      std::deque<std::unique_ptr<TimedTask>> delayedQ_;
      std::unique_ptr<std::thread> t_{nullptr};
      std::mutex mutex_;
      std::condition_variable cond_;

      bool running_{false};
      bool gracefulStopping_{false};
      bool useStdAsync_;
  };

} /* end of namespace: nul */

#endif /* end of include guard: TASK_QUEUE_H_ */
