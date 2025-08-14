#pragma once

#ifdef TEA_DEBUG_BUILD
#include "tea_list.h"
#endif

#include <stddef.h>

/**
 * @brief Function pointer type for custom memory allocation function.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
typedef void *(*tea_malloc_func_t)(size_t size);

/**
 * @brief Function pointer type for custom memory deallocation function.
 * @param ptr Pointer to the memory block to free.
 */
typedef void (*tea_free_func_t)(void *ptr);

#define tea_malloc(size) _tea_malloc(__FILE__, __LINE__, size)
#define tea_strdup(str)  _tea_strdup(__FILE__, __LINE__, str)

/**
 * @brief Structure to store source code location information.
 */
typedef struct tea_source_location_t {
  const char *file;   /**< Source file name */
  unsigned long line; /**< Source line number */
} tea_source_location_t;

#ifdef TEA_DEBUG_BUILD
/**
 * @brief Header structure prepended to memory allocations in debug builds.
 *        Contains metadata for tracking memory leaks.
 */
typedef struct tea_memory_header_t {
  tea_list_entry_t link;
  tea_source_location_t source_location;
  unsigned long size;
} tea_memory_header_t;
#endif

/**
 * @brief Internal memory allocation function.
 * @param file Source file name where the allocation was requested.
 * @param line Source line number where the allocation was requested.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 * @note Users should typically use the tea_malloc() macro instead.
 */
void *_tea_malloc(const char *file, unsigned long line, unsigned long size);

/**
 * @brief Internal string duplication function.
 * @param file Source file name where the duplication was requested.
 * @param line Source line number where the duplication was requested.
 * @param str The string to duplicate.
 * @return A pointer to the newly allocated string copy, or NULL on failure.
 * @note Users should typically use the tea_strdup() macro instead.
 */
char *_tea_strdup(const char *file, unsigned long line, const char *str);

/**
 * @brief Frees memory previously allocated by tea_malloc().
 * @param data Pointer to the memory block to free.
 */
void tea_free(void *data);

/**
 * @brief Initializes the tea memory management subsystem.
 *        Must be called before any tea_malloc() or tea_free() calls.
 *        In debug builds, initializes the allocation tracking list.
 * @param malloc_func Custom malloc function (NULL to use standard malloc)
 * @param free_func Custom free function (NULL to use standard free)
 */
void tea_memory_init(tea_malloc_func_t malloc_func, tea_free_func_t free_func);

/**
 * @brief Cleans up the tea memory management subsystem.
 *        Should be called at program termination.
 *        In debug builds, checks for memory leaks and reports them to stderr.
 */
void tea_memory_cleanup();
