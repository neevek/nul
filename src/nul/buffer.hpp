/*******************************************************************************
**          File: buffer.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-12 Thu 12:28 PM
**   Description: data buffer with the same layout as uv_buf_t 
*******************************************************************************/
#ifndef NUL_BUFFER_H_
#define NUL_BUFFER_H_ 
#include <cstdlib>
#include <cstring>

namespace nul {
  class Buffer final {
    public:
      struct Pod {
        char *data_;
        std::size_t len_;
        std::size_t capacity_;
      };

      Buffer(std::size_t capacity) {
        pod_.len_ = 0;
        pod_.capacity_ = capacity;
        pod_.data_ = new char[capacity];
      }

      ~Buffer() {
        delete [] pod_.data_;
      }

      void assign(const char *data, std::size_t len) {
        memcpy(pod_.data_, data, len);
        pod_.len_ = len;
      }

      char *getData() const {
        return pod_.data_;
      }

      std::size_t getLength() const {
        return pod_.len_;
      }

      void setLength(std::size_t len) {
        pod_.len_ = len;
      }

      std::size_t getCapacity() const {
        return pod_.capacity_;
      }

      Pod *asPod() {
        return &pod_;
      }

    private:
      Pod pod_;
  };

} /* end of namspace: nul */

#endif /* end of include guard: NUL_BUFFER_H_ */
