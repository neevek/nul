/*******************************************************************************
**          File: circular_buffer.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-04-12 Fri 03:00 PM
*******************************************************************************/
#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_
#include <array>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace nul {
  template <typename T, std::size_t MAX_SIZE>
  class CircularBuffer final {
    public:
      bool put(T data) {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        if (interrupted_) {
          return false;
        }
        if (size_ == MAX_SIZE) {
          cond_.wait(lock, [&](){ return interrupted_ || size_ < MAX_SIZE; });
          if (interrupted_) {
            return false;
          }
        }
        arr_[head_] = std::move(data);
        head_ = ++head_ % MAX_SIZE;
        ++size_;
        cond_.notify_one();
        return true;
      }

      T take(int waitTimeMillis = -1) {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        if (size_ == 0) {
          if (interrupted_) {
            return T{};
          }
          if (waitTimeMillis < 0) {
            cond_.wait(lock, [&](){ return interrupted_ || size_ > 0; });
          } else if (waitTimeMillis == 0) {
            return T{};         // don't wait
          } else {
            cond_.wait_for(lock, std::chrono::milliseconds(waitTimeMillis));
          }
          if (interrupted_) {
            return T{};
          }
        }

        return internalTakeOrDefault(lock);
      }

      T takeOrDefault() {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        return internalTakeOrDefault(lock);
      }

      std::size_t size() { 
        auto lock = std::unique_lock<std::mutex>(mutex_);
        return size_;
      }

      bool empty() { 
        return size() == 0;
      }

      constexpr std::size_t capacity() const { 
        return MAX_SIZE;
      }

      bool interrupted() {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        return interrupted_;
      }

      bool interruptedAndEmpty() {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        return interrupted_ && size_ == 0;
      }

      // once interrupted, the queue will no longer accept put
      void interrupt() {
        auto lock = std::unique_lock<std::mutex>(mutex_);
        interrupted_ = true;
        cond_.notify_all();
      }

    private:
      T internalTakeOrDefault(std::unique_lock<std::mutex> &lock) {
        if (size_ > 0) {
          T data = std::move(arr_[tail_]);
          tail_ = ++tail_ % MAX_SIZE;
          --size_;

          lock.unlock();
          cond_.notify_one();
          return data;
        }

        return T{};
      }
    
    private:
      std::array<T, MAX_SIZE> arr_;
      std::size_t head_{0};
      std::size_t tail_{0};
      std::size_t size_{0};

      std::condition_variable cond_;
      std::mutex mutex_;

      bool interrupted_{false};
  };
} /* end of namespace: nul */

#endif /* end of include guard: CIRCULAR_BUFFER_H_ */
