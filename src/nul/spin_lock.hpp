/*******************************************************************************
**          File: spin_lock.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2019-09-16 Mon 02:45 PM
**   Description: use std::atomic_flag to implement spin lock
*******************************************************************************/
#ifndef SPIN_LOCK_H_
#define SPIN_LOCK_H_
#include <atomic>

namespace nul {
  class SpinLock final {
    public:
      explicit SpinLock(std::atomic_flag &lock) : lock_(lock) {
        while (lock_.test_and_set(std::memory_order_acquire));
      }

      ~SpinLock() {
        lock_.clear(std::memory_order_release);
      }
    
    private:
      std::atomic_flag &lock_;
  };
} /* end of namespace: nul */

#endif /* end of include guard: SPIN_LOCK_H_ */
