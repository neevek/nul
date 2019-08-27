#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nul/task_queue.hpp"
#include "nul/log.hpp"
#include <stdio.h>

using namespace nul;
using namespace std::chrono;

int64_t getCurrentTime() {
  return duration_cast<milliseconds>(
    system_clock::now().time_since_epoch()).count();
}

TEST(TaskQueue, Test) {
  TaskQueue q{true};
  q.start();

  auto count = 0;
  auto delay = 500;

  {
    q.postDelayed(delay, [&count](int64_t delayedTime){
      EXPECT_GE(getCurrentTime(), delayedTime);
      ++count;
    }, getCurrentTime() + delay);

    q.post([&count]{
      ++count;
    });

    q.postDelayed(delay + 5, [&count](){
      EXPECT_EQ(count, 2);
    });
  }

  std::this_thread::sleep_for(milliseconds(800));

  count = 0;
  delay = 100;
  auto repeat = 100;
  q.postAtInterval(repeat, repeat, [&count]{
    ++count;
  });

  q.postDelayed(delay * 5 + 5, [&count](){
    EXPECT_EQ(count, 5);
  });

  std::this_thread::sleep_for(milliseconds(800));

  q.stop();
}
