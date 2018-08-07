/*******************************************************************************
**          File: buffer_pool.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2018-07-26 Thu 08:08 PM
**   Description: the buffer pool 
*******************************************************************************/
#ifndef NUL_BUFFER_POOL_H_
#define NUL_BUFFER_POOL_H_
#include "buffer.hpp"
#include <deque>
#include <memory>
#include <algorithm>

namespace nul {
  class BufferPool {
    public:
      BufferPool(std::size_t maxBufferSize, std::size_t maxBufferCount) :
        maxBufferSize_(maxBufferSize), maxBufferCount_(maxBufferCount) {
        }
      virtual ~BufferPool() = default;

      std::unique_ptr<Buffer> requestBuffer(std::size_t size) {
        if (!freeBuffers_.empty()) {
          auto freeBufIt = std::find_if(
            freeBuffers_.begin(), freeBuffers_.end(), [size](auto &buf) {
              return buf->getCapacity() >= size;
            });
          if (freeBufIt != freeBuffers_.end()) {
            auto freeBuf = std::move(*freeBufIt);
            freeBuffers_.erase(freeBufIt);
            return freeBuf;
          }
        }
        return std::make_unique<Buffer>(size);
      }

      void returnBuffer(std::unique_ptr<Buffer> &&data) {
        if (data->getCapacity() > maxBufferSize_ &&
            freeBuffers_.size() < maxBufferCount_) {
          freeBuffers_.push_back(std::move(data));
        }
      }

      std::unique_ptr<Buffer> assembleDataBuffer(
        const char *data, std::size_t dataLen) {
        auto dataBuf = requestBuffer(dataLen);
        dataBuf->assign(data, dataLen);
        return dataBuf;
      }

    private:
      std::deque<std::unique_ptr<Buffer>> freeBuffers_;
      std::size_t maxBufferSize_;
      std::size_t maxBufferCount_;
  };
} /* end of namspace: nul */

#endif /* end of include guard: NUL_BUFFER_POOL_H_ */
