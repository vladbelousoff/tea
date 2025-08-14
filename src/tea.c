#include "tea.h"

#include "tea_memory.h"

void tea_init(tea_malloc_func_t malloc_func, tea_free_func_t free_func)
{
  tea_memory_init(malloc_func, free_func);
}

void tea_cleanup()
{
  tea_memory_cleanup();
}
