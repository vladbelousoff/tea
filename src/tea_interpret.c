#include "tea_interpret.h"

#include "tea_function.h"
#include "tea_scope.h"
#include "tea_struct.h"

#include <rtl_memory.h>

void tea_interpret_init(tea_context_t* context, const char* filename)
{
  context->filename = filename;
  rtl_list_init(&context->functions);
  rtl_list_init(&context->native_functions);
  rtl_list_init(&context->struct_declarations);
  rtl_list_init(&context->trait_declarations);
  rtl_list_init(&context->trait_implementations);
  rtl_list_init(&context->variable_pool);
}

void tea_interpret_cleanup(const tea_context_t* context)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->functions)
  {
    tea_function_t* function = rtl_list_record(entry, tea_function_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &context->native_functions)
  {
    tea_native_function_t* function = rtl_list_record(entry, tea_native_function_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &context->struct_declarations)
  {
    tea_struct_declaration_t* struct_declaration =
      rtl_list_record(entry, tea_struct_declaration_t, link);
    rtl_list_remove(entry);

    rtl_list_entry_t* function_entry;
    rtl_list_entry_t* function_entry_safe;
    rtl_list_for_each_safe(function_entry, function_entry_safe, &struct_declaration->functions)
    {
      tea_function_t* function = rtl_list_record(function_entry, tea_function_t, link);
      rtl_list_remove(function_entry);
      rtl_free(function);
    }

    rtl_free(struct_declaration);
  }

  rtl_list_for_each_safe(entry, safe, &context->variable_pool)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    rtl_free(variable);
  }
}
