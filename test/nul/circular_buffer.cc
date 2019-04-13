#include <gtest/gtest.h>
#include "nul/circular_buffer.hpp"
#include "nul/log.hpp"
#include <future>
#include <thread>

#define ENABLE_PROFILING
#include "nul/profiler.hpp"

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
  PROFILE_TIME_COST_USEC("ConcurrentAccess");
  constexpr auto MAX_SIZE = 5;
  nul::CircularBuffer<int, MAX_SIZE> cbuf;

  auto f1 = std::async(std::launch::async, [&](){
    for (int i = 0; i < 10; ++i) {
      cbuf.put(i);
    }
  });

  auto f2 = std::async(std::launch::async, [&](){
    for (int i = 0; i < 10; ++i) {
      LOG_D("take, size=%lu, data=%d", cbuf.size(), cbuf.take(100));
    }
  });

  f2.get();
  //f1.get();
}

TEST(CircularBuffer, UniquePointer) {
  PROFILE_TIME_COST_USEC("UniquePointer");

  constexpr auto MAX_SIZE = 5;
  nul::CircularBuffer<std::unique_ptr<int>, MAX_SIZE> cbuf;

  auto f1 = std::async(std::launch::async, [&](){
    for (int i = 0; i < 100; ++i) {
      cbuf.put(std::make_unique<int>(i));
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });

  auto f2 = std::async(std::launch::async, [&](){
    int n = 100;
    while (n > 0) {
      auto p = cbuf.take(5);
      if (p) {
        --n;
        LOG_D("take, size=%lu, data=%d", cbuf.size(), *p);
      } else {
        LOG_W("take returns null, size=%lu", cbuf.size());
      }
    }
  });

  f2.get();
  //f1.get();
}
