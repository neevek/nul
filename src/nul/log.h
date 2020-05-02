#ifndef NUL_LOG_H_
#define NUL_LOG_H_
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GLOBAL_LOG_TAG
#define GLOBAL_LOG_TAG "-"
#endif

extern int gLogVerboseInDebugBuild;

static inline void setLogVerboseInDebugBuild(int enable) {
  gLogVerboseInDebugBuild = enable > 0 ? 1 : 0;
}

#define _TIME_BUFFER_SIZE 24
static char *doStrftime_(char *buffer) {
  struct timeval now;
  gettimeofday(&now, NULL);

  size_t len = strftime(buffer, _TIME_BUFFER_SIZE, "%Y-%m-%d %H:%M:%S.",
      localtime(&now.tv_sec));
  int milli = now.tv_usec / 1000;
  sprintf(buffer + len, "%03d", milli);

  return buffer;
}

// Android uses the following enum values for log levels, but enums cannot be
// used for comparison during preprocessing time, their values are hard coded
// like so, see <android/log.h> for the enum declarations.
#define LOG_LEVEL_VERBOSE   2
#define LOG_LEVEL_DEBUG     3
#define LOG_LEVEL_INFO      4
#define LOG_LEVEL_WARN      5
#define LOG_LEVEL_ERROR     6

#ifdef LOG_HIDE_FILENAME
#define FILENAME_INFO "?"
#else
#if !defined(__FILENAME__)
#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? \
   strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#define FILENAME_INFO __FILENAME__
#endif

#ifdef LOG_HIDE_FUNCTION_NAME
#define FUNCTION_INFO "?"
#else
#define FUNCTION_INFO __FUNCTION__
#endif

inline static const char *logLevelStr_(int level) {
  switch(level) {
    case LOG_LEVEL_VERBOSE: return "V";
    case LOG_LEVEL_DEBUG: return "D";
    case LOG_LEVEL_INFO: return "I";
    case LOG_LEVEL_WARN: return "W";
    case LOG_LEVEL_ERROR: return "E";
    default: return "";
  }
}

// log to file
#if defined(LOG_TO_FILE) && defined(LOG_FILE_PATH)
#define DO_LOG_(level, color, fmt, ...) do { \
  FILE *f = fopen(LOG_FILE_PATH, "a+"); \
  char buf[_TIME_BUFFER_SIZE];  \
  fprintf(f, "%s %s [%s] [%s:%d] %s - " fmt "\n", \
      doStrftime_(buf), GLOBAL_LOG_TAG, logLevelStr_(level), \
      FILENAME_INFO, __LINE__, FUNCTION_INFO, ##__VA_ARGS__); \
  fclose(f); \
} while (0)

// log to Android logcat
#elif __ANDROID__
#define DO_LOG_(level, color, fmt, ...) do { \
  __android_log_print(level, GLOBAL_LOG_TAG, "[%s:%d] %s - " fmt "\n", \
      FILENAME_INFO, __LINE__, FUNCTION_INFO, ##__VA_ARGS__); \
} while (0)

#else
#ifndef NO_TERM_COLOR
#define NO_TERM_COLOR
#define KNRM "\x1B[0m"
#define KBLU "\x1b[34m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[92m"
#define KYEL "\x1B[93m"
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

// log to stderr
#define DO_LOG_(level, color, fmt, ...) do { \
  char buf[_TIME_BUFFER_SIZE];  \
  fprintf(stderr, color "%s %s [%s] [%s:%d] %s - " fmt KEND "\n", \
      doStrftime_(buf), GLOBAL_LOG_TAG, logLevelStr_(level), FILENAME_INFO, \
      __LINE__, FUNCTION_INFO, ##__VA_ARGS__); \
} while (0)
#endif

#define LOG_V_WITH_CONDITIONAL(fmt, ...) \
  do { \
    if (gLogVerboseInDebugBuild) \
      DO_LOG_(LOG_LEVEL_VERBOSE, KNRM, fmt, ##__VA_ARGS__); \
  } while(0)

#ifndef LOG_LEVEL
#ifdef LOG_VERBOSE
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#elif LOG_DEBUG
#define LOG_LEVEL LOG_LEVEL_DEBUG
#elif LOG_INFO
#define LOG_LEVEL LOG_LEVEL_INFO
#elif LOG_WARN
#define LOG_LEVEL LOG_LEVEL_WARN
#elif LOG_ERROR
#define LOG_LEVEL LOG_LEVEL_ERROR
#else
#define LOG_LEVEL (LOG_LEVEL_ERROR+1)
#endif
#endif

/* enable LOG_V if defined(DEBUG), and control output with runtime flag */
#if LOG_LEVEL <= LOG_LEVEL_VERBOSE || defined(DEBUG)

#if LOG_LEVEL > LOG_LEVEL_VERBOSE
#define LOG_V(fmt, ...) LOG_V_WITH_CONDITIONAL(fmt, ##__VA_ARGS__)
#else
#define LOG_V(fmt, ...) DO_LOG_(LOG_LEVEL_VERBOSE, KNRM, fmt, ##__VA_ARGS__)
#endif
#define TLOG_V(extraTag, fmt, ...) LOG_V("[%s] " fmt, extraTag, ##__VA_ARGS__)

#else
#define LOG_V(fmt, ...)
#define TLOG_V(extraTag, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_D(fmt, ...) DO_LOG_(LOG_LEVEL_DEBUG, KBLU, fmt, ##__VA_ARGS__)
#define TLOG_D(extraTag, fmt, ...) LOG_D("[%s] " fmt, extraTag, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...)
#define TLOG_D(extraTag, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_I(fmt, ...) DO_LOG_(LOG_LEVEL_INFO, KGRN, fmt, ##__VA_ARGS__)
#define TLOG_I(extraTag, fmt, ...) LOG_I("[%s] " fmt, extraTag, ##__VA_ARGS__)
#else
#define LOG_I(fmt, ...)
#define TLOG_I(extraTag, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_W(fmt, ...) DO_LOG_(LOG_LEVEL_WARN, KYEL, fmt, ##__VA_ARGS__)
#define TLOG_W(extraTag, fmt, ...) LOG_W("[%s] " fmt, extraTag, ##__VA_ARGS__)
#else
#define LOG_W(fmt, ...)
#define TLOG_W(extraTag, fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_E(fmt, ...) DO_LOG_(LOG_LEVEL_ERROR, KRED, fmt, ##__VA_ARGS__)
#define TLOG_E(extraTag, fmt, ...) LOG_E("[%s] " fmt, extraTag, ##__VA_ARGS__)
#else
#define LOG_E(fmt, ...)
#define TLOG_E(extraTag, fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: NUL_LOG_H_ */
