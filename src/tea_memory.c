#include "tea_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TEA_DEBUG_BUILD
#include "tea_log.h"
#endif

#ifdef TEA_DEBUG_BUILD
/**
 * @internal
 * @brief Head of the linked list used to track memory allocations in debug builds.
 */
static tea_list_entry_t tea_memory_allocations;
#endif

/**
 * @internal
 * @brief Wrapper function for standard malloc to match our function pointer signature.
 */
static void *default_malloc_wrapper(size_t size)
{
  return malloc(size);
}

/**
 * @internal
 * @brief Wrapper function for standard free to match our function pointer signature.
 */
static void default_free_wrapper(void *ptr)
{
  free(ptr);
}

/**
 * @internal
 * @brief Global function pointers for custom memory allocators.
 */
static tea_malloc_func_t g_malloc_func = NULL;
static tea_free_func_t g_free_func = NULL;

void *_tea_malloc(const char *file, unsigned long line, unsigned long size)
{
#ifdef TEA_DEBUG_BUILD
  // It's not a memory leak, it's just a trick to add a bit more
  // info about allocated memory so...
  // ReSharper disable once CppDFAMemoryLeak
  char *data = (char *)g_malloc_func(sizeof(tea_memory_header_t) + size);

  if (data == NULL) {
    return NULL;
  }

  tea_memory_header_t *header = (tea_memory_header_t *)data;
  header->source_location.file = file;
  header->source_location.line = line;
  header->size = size;
  tea_list_add_tail(&tea_memory_allocations, &header->link);
  // Mark the memory with 0x77 to be able to debug uninitialized memory
  memset(&data[sizeof(tea_memory_header_t)], 0x77, size);
  // Return only the needed piece and hide the header
  // ReSharper disable once CppDFAMemoryLeak
  return &data[sizeof(tea_memory_header_t)];
#else
  return g_malloc_func(size);
#endif
}

char *_tea_strdup(const char *file, unsigned long line, const char *str)
{
  if (str == NULL) {
    return NULL;
  }

  const size_t len = strlen(str) + 1; // +1 for null terminator
  char *copy = _tea_malloc(file, line, len);
  if (copy != NULL) {
    strcpy(copy, str);
  }

  return copy;
}

void tea_free(void *data)
{
  if (data == NULL) {
    return;
  }

#ifdef TEA_DEBUG_BUILD
  // Find the header with meta information
  tea_memory_header_t *header =
    (tea_memory_header_t *)((char *)data - sizeof(tea_memory_header_t));
  tea_list_remove(&header->link);
  // Now we can free the real allocated piece
  g_free_func(header);
#else
  g_free_func(data);
#endif
}

void tea_memory_init(tea_malloc_func_t malloc_func, tea_free_func_t free_func)
{
  // Set custom allocators or default to standard malloc/free wrappers
  g_malloc_func = malloc_func ? malloc_func : default_malloc_wrapper;
  g_free_func = free_func ? free_func : default_free_wrapper;

#ifdef TEA_DEBUG_BUILD
  tea_list_init(&tea_memory_allocations);
#endif
}

void tea_memory_cleanup()
{
#ifdef TEA_DEBUG_BUILD
  tea_list_entry_t *entry;
  tea_list_entry_t *safe;
  tea_list_for_each_safe(entry, safe, &tea_memory_allocations)
  {
    tea_memory_header_t *header =
      tea_list_record(entry, tea_memory_header_t, link);
    tea_log_err("Leaked memory, file: %s, line: %lu, size: %lu",
                header->source_location.file, header->source_location.line,
                header->size);
    tea_list_remove(&header->link);
    g_free_func(header);
  }
#endif
}
