/*******************************************************************************
**          File: Profiler.h
**        Author: neevek <i@neevek.net>.
** Creation Time: 2017-07-12
**   Description: utility for profiling code running time
*******************************************************************************/
#ifndef PROFILER_H_
#define PROFILER_H_
#include <chrono>
#include <string>
#include <cinttypes>
#include <type_traits>

#include "log.hpp"

#ifndef LOG_TAG_NAME
#define LOG_TAG_NAME ""
#endif

namespace nul {

  template <typename T>
  struct is_valid_time_unit {
    static constexpr bool value = false;
  };

  template <typename Rep, typename Period>
  struct is_valid_time_unit<std::chrono::duration<Rep, Period>> {
    static constexpr bool value = true;
  };

  template <typename TimeUnit>
  class Profiler {
    static_assert(is_valid_time_unit<TimeUnit>::value, "Invalid TimeUnit");

    public:
    Profiler(
        const char *filename, const char *function_name,
        int line_num, const char *fmt, ...) :
      filename_(filename),
      function_name_(function_name),
      line_num_(line_num),
      begin_time_(std::chrono::high_resolution_clock::now()) {

      va_list args;
      va_start (args, fmt);
      char buf[512];
      vsnprintf(buf, 512, fmt, args);
      va_end (args);

      msg_.append(buf);
    }

    ~Profiler() {
      using namespace std::chrono;

      auto duration = duration_cast<TimeUnit>(
          high_resolution_clock::now() - begin_time_).count();
      const char *unit_str = 
        (std::is_same<TimeUnit, milliseconds>::value ? "ms" :
        (std::is_same<TimeUnit, microseconds>::value ? "us" :
        (std::is_same<TimeUnit, nanoseconds>::value  ? "ns" :
        (std::is_same<TimeUnit, seconds>::value      ? "s"  :
        (std::is_same<TimeUnit, minutes>::value      ? "m"  :
        (std::is_same<TimeUnit, hours>::value        ? "h"  : ""))))));
#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_INFO, LOG_TAG_NAME, "[%s:%d] %s - %s, time_cost: %lli %s\n",
          filename_, line_num_, function_name_, msg_.c_str(), duration, unit_str);
#else
      char buf[TIME_BUFFER_SIZE];
      fprintf(stderr, KBLU "%s %s [I] [%s#%d] %s - %s, time_cost: " KEND KYEL "%lli" KEND KBLU " %s\n" KEND,
          log_strtime(buf), LOG_TAG_NAME, filename_, line_num_,
          function_name_, msg_.c_str(), duration, unit_str);
#endif
    }

  private:
    std::string msg_;
    const char *filename_{nullptr}; // must be string literal
    const char *function_name_{nullptr}; // must be string literal
    int line_num_{0};
    std::chrono::high_resolution_clock::time_point begin_time_;
  };

  using TimeCostCalcMsec = Profiler<std::chrono::milliseconds>;
  using TimeCostCalcUsec = Profiler<std::chrono::microseconds>;
  using TimeCostCalcNsec = Profiler<std::chrono::nanoseconds>;
  using TimeCostCalcHour = Profiler<std::chrono::hours>;
  using TimeCostCalcMinute = Profiler<std::chrono::minutes>;
  using TimeCostCalcSec = Profiler<std::chrono::seconds>;
} /* end of namespace: nul */

#define EXPAND_(a, b) a ## b
#define COMBINE_(a, b) EXPAND_(a, b)

#ifdef ENABLE_PROFILING
#define PROFILE_TIME_COST(time_unit, fmt, ...)\
    nul::Profiler<time_unit> \
    COMBINE_(__t, __LINE__) (__FILENAME__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define PROFILE_TIME_COST(time_unit, fmt, ...)
#endif

#define PROFILE_TIME_COST_USEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::microseconds, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_COST_MSEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::milliseconds, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_COST_NSEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::nanoseconds, fmt, ##__VA_ARGS__)

#endif //PROFILER_H_

