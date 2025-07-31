#include "tea_scope.h"
#include "tea_expression.h"

#include <stdlib.h>
#include <string.h>

#include <rtl.h>
#include <rtl_log.h>
#include <rtl_memory.h>

#define TEA_VARIABLE_POOL_ENABLED 1

tea_variable_t* tea_allocate_variable(const tea_context_t* context)
{
#if TEA_VARIABLE_POOL_ENABLED
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->variable_pool)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    return variable;
  }
#endif
  return rtl_malloc(sizeof(tea_variable_t));
}

void tea_free_variable(tea_context_t* context, tea_variable_t* variable)
{
#if TEA_VARIABLE_POOL_ENABLED
  if (variable) {
    rtl_list_add_head(&context->variable_pool, &variable->link);
  }
#else
  rtl_free(variable);
#endif
}

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent_scope)
{
  scope->parent_scope = parent_scope;
  rtl_list_init(&scope->variables);
}

void tea_scope_cleanup(tea_context_t* context, const tea_scope_t* scope)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &scope->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    tea_free_variable(context, variable);
  }
}

tea_variable_t* tea_scope_find_variable_local(const tea_scope_t* scope, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &scope->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    const char* variable_name = variable->name;
    if (!variable_name) {
      continue;
    }
    if (!strcmp(variable_name, name)) {
      return variable;
    }
  }

  return NULL;
}

tea_variable_t* tea_scope_find_variable(const tea_scope_t* scope, const char* name)
{
  const tea_scope_t* current_scope = scope;
  while (current_scope) {
    tea_variable_t* variable = tea_scope_find_variable_local(current_scope, name);
    if (variable) {
      return variable;
    }

    current_scope = current_scope->parent_scope;
  }

  return NULL;
}

bool tea_declare_variable(tea_context_t* context, tea_scope_t* scope, const char* name,
  const unsigned int flags, const char* type, const tea_ast_node_t* initial_value)
{
  tea_variable_t* variable = tea_scope_find_variable_local(scope, name);
  if (variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' is already declared in current scope - redeclaration not "
      "allowed",
      variable->name);
    return false;
  }

  variable = tea_allocate_variable(context);
  if (!variable) {
    rtl_log_err("Memory error: Failed to allocate memory for variable '%s'", name);
    return false;
  }

  variable->name = name;
  variable->flags = flags;
  tea_value_t value = tea_interpret_evaluate_expression(context, scope, initial_value);
  if (value.type == TEA_VALUE_INVALID) {
    tea_free_variable(context, variable);
    return false;
  }
  if (type) {
    const tea_value_type_t predefined_type = tea_value_get_type_by_string(type);
    if (predefined_type == TEA_VALUE_INVALID) {
      rtl_log_err("Runtime error: Unknown type '%s' specified in variable declaration", type);
      tea_free_variable(context, variable);
      return false;
    }
    if (value.type == TEA_VALUE_NULL) {
      value.null_type = predefined_type;
    } else if (value.type != predefined_type) {
      rtl_log_err(
        "Runtime error: Type mismatch - value type does not match declared variable type");
      tea_free_variable(context, variable);
      return false;
    }
  }
  variable->value = value;

  switch (variable->value.type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("Declare variable %s : %s = %d", name,
        tea_value_get_type_string(variable->value.type), variable->value.i32);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("Declare variable %s : %s = %f", name,
        tea_value_get_type_string(variable->value.type), variable->value.f32);
      break;
    case TEA_VALUE_NULL:
      rtl_log_dbg(
        "Declare variable %s : %s = null", name, tea_value_get_type_string(variable->value.type));
    default:
      break;
  }

  rtl_list_add_tail(&scope->variables, &variable->link);

  return true;
}

#undef TEA_VARIABLE_POOL_ENABLED
