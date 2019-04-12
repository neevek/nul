#include <gtest/gtest.h>
#include "nul/circular_buffer.hpp"
#include "nul/log.hpp"
#include <future>

using namespace nul;

TEST(CircularBuffer, Test) {
  constexpr auto MAX_SIZE = 3;
  nul::CircularBuffer<int, MAX_SIZE> cbuf;
  ASSERT_TRUE(cbuf.size() == 0);
  ASSERT_TRUE(cbuf.empty());
  ASSERT_TRUE(cbuf.capacity() == MAX_SIZE);

  cbuf.put(1);
  ASSERT_TRUE(cbuf.size() == 1);
  ASSERT_TRUE(!cbuf.empty());
  cbuf.put(2);
  cbuf.put(3);
  ASSERT_TRUE(cbuf.size() == 3);

  ASSERT_TRUE(cbuf.take() == 1);
  cbuf.put(4);
  ASSERT_TRUE(cbuf.size() == 3);
  ASSERT_TRUE(cbuf.take() == 2);
}

TEST(CircularBuffer, ConcurrentAccess) {
  constexpr auto MAX_SIZE = 5;
  nul::CircularBuffer<int, MAX_SIZE> cbuf;

  auto f1 = std::async(std::launch::async, [&](){
    for (int i = 0; i < 1000; ++i) {
      cbuf.put(i);
    }
  });

  auto f2 = std::async(std::launch::async, [&](){
    for (int i = 0; i < 1000; ++i) {
      LOG_D("take, size=%lu, data=%d", cbuf.size(), cbuf.take());
    }
  });

  f2.get();
  //f1.get();
}

