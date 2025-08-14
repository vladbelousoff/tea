#pragma once

#ifdef TEA_DEBUG_BUILD
#include "tea_log.h"
#endif

#include "tea_memory.h"

#include <stdlib.h>

/**
 * @brief Assert macro that logs an error and aborts if condition is false.
 * @param condition The condition to check
 * @param message Optional error message (can be a format string with args)
 */
#define tea_assert(condition, message, ...)                                    \
  do {                                                                         \
    if (!(condition)) {                                                        \
      tea_log_err("Assertion failed: %s - " message, #condition,               \
                  ##__VA_ARGS__);                                              \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/**
 * @brief Initializes the runtime library subsystems.
 *        Must be called once at the start of the application.
 * @param malloc_func Custom malloc function (NULL to use standard malloc)
 * @param free_func Custom free function (NULL to use standard free)
 */
void tea_init(tea_malloc_func_t malloc_func, tea_free_func_t free_func);

/**
 * @brief Cleans up the runtime library subsystems.
 *        Should be called once at the end of the application.
 */
void tea_cleanup();
