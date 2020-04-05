/*******************************************************************************
**          File: xbuffer.hpp
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-08-06 Mon 04:40 PM
**   Description: xbuffer wraps buffer object with specified length, take()
**                returns the buffer only the length grows to the specified length
*******************************************************************************/
#ifndef XBUFFER_H_
#define XBUFFER_H_
#include "buffer.hpp"
#include "log.h"
#include <memory>
#include <string>
#include <deque>
#include <type_traits>

namespace nul {
  template <uint8_t DATA_LENGTH_BYTES>
  class XBuffer {
    static_assert(
      DATA_LENGTH_BYTES <= 4,
      "DATA_LENGTH_BYTES cannot be greater than 4");

    public:
      /**
       * the first 'dataLengthBytes' bytes of data will be treated as a
       * 16bit big-endian integer, denoting the length of the data that follow
       */
      void offer(const char *data, std::size_t len) {
        data_.append(data, len);

        while (true) {
          if (dataLen_ == 0) {
            if (data_.size() < dataLengthBytes_) {
              break;
            }

            auto d = data_.data();
            int dlBytes = dataLengthBytes_;
            while (--dlBytes >= 0) {
              auto shift = 8 * dlBytes;
              dataLen_ += (*d << shift) & (0xff << shift);
              ++d;
            }
          }

          if (data_.size() - dataLengthBytes_ >= dataLen_) {
            auto buf = std::make_unique<Buffer>(dataLen_);
            if (dataLen_) {
              buf->assign(data_.data() + dataLengthBytes_, dataLen_);
            }
            q_.push_back(std::move(buf));

            auto totalLen = dataLen_ + dataLengthBytes_;
            if (data_.size() == totalLen) {
              data_.clear();
            } else {
              data_.erase(0, totalLen);
            }
            dataLen_ = 0;

          } else {
            break;
          }
        }
      }

      std::unique_ptr<Buffer> take() {
        auto b = std::move(q_.front());
        q_.pop_front();
        return std::move(b);
      }

      std::size_t getBufferCount() const {
        return q_.size();
      }

      bool empty() const {
        return q_.empty();
      }

      void clear() {
        dataLen_ = 0;
        data_.clear();
        q_.clear();
      }
    
    private:
      uint8_t dataLengthBytes_{DATA_LENGTH_BYTES};

      std::size_t dataLen_{0};
      std::string data_;
      std::deque<std::unique_ptr<Buffer>> q_;
  };
} /* end of namspace: nul */

#endif /* end of include guard: XBUFFER_H_ */
