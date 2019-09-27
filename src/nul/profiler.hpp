/*******************************************************************************
**          File: profiler.hpp
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
#include <time.h>
#include <sys/time.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifndef NO_TERM_COLOR
#define NO_TERM_COLOR
#define KNRM  "\x1B[0m"
#define KBLU  "\x1b[34m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[92m"
#define KYEL  "\x1B[93m"
#define KEND  KNRM
#else
#ifndef KNRM
#define KNRM
#define KBLU
#define KRED
#define KGRN
#define KYEL
#define KEND
#endif
#endif

#ifndef LOG_TAG_NAME
#define LOG_TAG_NAME "TIME_PROFILE"
#endif

#ifndef __FILENAME__
#define __FILENAME__\
  (strrchr(__FILE__, '/') ?\
   strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

namespace {
  inline char *log_strtime(char *buffer, int buf_size) {
    struct timeval now;
    gettimeofday(&now, NULL);

    size_t len = strftime(buffer, buf_size, "%Y-%m-%d %H:%M:%S.",
                          localtime(&now.tv_sec));
    int milli = now.tv_usec / 1000;
    sprintf(buffer + len, "%03d", milli);

    return buffer;
  }
}

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
      

    public:
      Profiler(
          const char *filename, const char *function_name, int line_num,
          const char *fmt, ...) :
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

      void setLogThreshold(TimeUnit log_threshold) {
        log_threshold_ = log_threshold;
      }

      void setLogToFilePath(const char *log_to_file_path) {
        log_to_file_path_ = log_to_file_path;
      }

      ~Profiler() {
        using namespace std::chrono;

        auto now = high_resolution_clock::now();
        auto dura = now - begin_time_;
        if (dura < log_threshold_) {
          return;
        }

        auto duration = duration_cast<TimeUnit>(dura).count();
        const char *unit_str = 
          (std::is_same<TimeUnit, milliseconds>::value ? "ms" :
          (std::is_same<TimeUnit, microseconds>::value ? "us" :
          (std::is_same<TimeUnit, nanoseconds>::value  ? "ns" :
          (std::is_same<TimeUnit, seconds>::value      ? "s"  :
          (std::is_same<TimeUnit, minutes>::value      ? "m"  :
          (std::is_same<TimeUnit, hours>::value        ? "h"  : ""))))));

        constexpr int timeBufSize = 24;

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG_NAME, "[%s:%d] %s - %s (time: %lli %s)\n",
            filename_, line_num_, function_name_, msg_.c_str(), duration, unit_str);
#else
        char timeBuf[timeBufSize];
        fprintf(stderr, KBLU "%s %s [I] [%s#%d] %s - %s (time: " KEND KYEL "%lli" KEND KBLU " %s)\n" KEND,
            log_strtime(timeBuf, timeBufSize), LOG_TAG_NAME, filename_, line_num_,
            function_name_, msg_.c_str(), duration, unit_str);
#endif

        if (log_to_file_path_) {
          FILE *f = fopen(log_to_file_path_, "a+");
          char timeBuf[timeBufSize];
          fprintf(f, "%s %s [I] [%s#%d] %s - %s (time: %lli %s)\n",
              log_strtime(timeBuf, timeBufSize), LOG_TAG_NAME, filename_, line_num_,
              function_name_, msg_.c_str(), duration, unit_str);
          fclose(f);
        }
      }

    private:
      std::string msg_;
      const char *filename_{nullptr}; // must be string literal
      const char *function_name_{nullptr}; // must be string literal
      int line_num_{0};
      std::chrono::high_resolution_clock::time_point begin_time_;
      TimeUnit log_threshold_{0};
      const char *log_to_file_path_{nullptr}; // log to file if not null
  };

  using ProfilerMsec = Profiler<std::chrono::milliseconds>;
  using ProfilerUsec = Profiler<std::chrono::microseconds>;
  using ProfilerNsec = Profiler<std::chrono::nanoseconds>;
  using ProfilerSec = Profiler<std::chrono::seconds>;
} /* end of namespace: nul */

#define PROFILE_TIME_COST(TimeUnit, log_threshold, log_to_file_path, fmt, ...)\
    do { \
      nul::Profiler<TimeUnit> \
      profile(__FILENAME__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
      profile.setLogThreshold(TimeUnit(log_threshold)); \
      profile.setLogToFilePath(log_to_file_path); \
    } while (0)

#define PROFILE_TIME_COST_USEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::microseconds, 0, nullptr, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_COST_MSEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::milliseconds, 0, nullptr, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_COST_NSEC(fmt, ...)\
    PROFILE_TIME_COST(std::chrono::nanoseconds, 0, nullptr, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_USEC(log_threshold, log_to_file_path, fmt, ...)\
    PROFILE_TIME_COST(std::chrono::microseconds, log_threshold, log_to_file_path, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_MSEC(log_threshold, log_to_file_path, fmt, ...)\
    PROFILE_TIME_COST(std::chrono::milliseconds, log_threshold, log_to_file_path, fmt, ##__VA_ARGS__)

#define PROFILE_TIME_NSEC(log_threshold, log_to_file_path, fmt, ...)\
    PROFILE_TIME_COST(std::chrono::nanoseconds, log_threshold, log_to_file_path, fmt, ##__VA_ARGS__)

#endif //PROFILER_H_
