#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>

// Time stamp functionality
static const char *tea_get_time_stamp(void)
{
  static char stamp[16];
  const time_t now = time(NULL);
  const struct tm *tm_info = localtime(&now);
  strftime(stamp, sizeof(stamp), "%H:%M:%S", tm_info);
  return stamp;
}

static const char *tea_filename(const char *filename)
{
#ifdef _WIN32
  const char *p = strrchr(filename, '\\');
#else
  const char *p = strrchr(filename, '/');
#endif
  if (p) {
    return p + 1;
  }

#ifdef _WIN32
  p = strrchr(filename, '/');
  if (p) {
    return p + 1;
  }
#endif

  return filename;
}

#define TEA_FILENAME tea_filename(__FILE__)

// ANSI color codes
#define TEA_COLOR_RESET  "\033[00m"
#define TEA_COLOR_YELLOW "\033[33m"
#define TEA_COLOR_RED    "\033[31m"
#define TEA_COLOR_GREEN  "\033[32m"
#define TEA_COLOR_WHITE  "\033[37m"

// Common log format string
#define TEA_LOG_FORMAT "[%-s|%-s] [%-16s:%5u] (%s) "

// Unified fprintf-based logging macro for colored format
#define _tea_printf_color(color, lvl, file, line, func, fmt, ...)              \
  fprintf(stdout, "%s" TEA_LOG_FORMAT "%s" fmt "\n", color, lvl,               \
          tea_get_time_stamp(), file, line, func, TEA_COLOR_RESET,             \
          ##__VA_ARGS__)

#if TEA_DEBUG_LEVEL >= 4
#define tea_log_inf(_fmt, ...)                                                 \
  _tea_printf_color(TEA_COLOR_WHITE, "INF", TEA_FILENAME, __LINE__,            \
                    __FUNCTION__, _fmt, ##__VA_ARGS__)
#else
#define tea_log_inf(_fmt, ...)                                                 \
  do {                                                                         \
  } while (0)
#endif

#if TEA_DEBUG_LEVEL >= 3
#define tea_log_dbg(_fmt, ...)                                                 \
  _tea_printf_color(TEA_COLOR_GREEN, "DBG", TEA_FILENAME, __LINE__,            \
                    __FUNCTION__, _fmt, ##__VA_ARGS__)
#else
#define tea_log_dbg(_fmt, ...)                                                 \
  do {                                                                         \
  } while (0)
#endif

#if TEA_DEBUG_LEVEL >= 2
#define tea_log_wrn(_fmt, ...)                                                 \
  _tea_printf_color(TEA_COLOR_YELLOW, "WRN", TEA_FILENAME, __LINE__,           \
                    __FUNCTION__, _fmt, ##__VA_ARGS__)
#else
#define tea_log_wrn(_fmt, ...)                                                 \
  do {                                                                         \
  } while (0)
#endif

#if TEA_DEBUG_LEVEL >= 1
#define tea_log_err(_fmt, ...)                                                 \
  _tea_printf_color(TEA_COLOR_RED, "ERR", TEA_FILENAME, __LINE__,              \
                    __FUNCTION__, _fmt, ##__VA_ARGS__)
#else
#define tea_log_err(_fmt, ...)                                                 \
  do {                                                                         \
  } while (0)
#endif
