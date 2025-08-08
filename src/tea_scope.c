#include "tea_scope.h"
#include "tea_expr.h"

#include <stdlib.h>
#include <string.h>

#include <rtl.h>
#include <rtl_log.h>
#include <rtl_memory.h>

#define TEA_VARIABLE_POOL_ENABLED 1

tea_var_t* tea_alloc_var(const tea_ctx_t* context)
{
#if TEA_VARIABLE_POOL_ENABLED
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->vars)
  {
    tea_var_t* variable = rtl_list_record(entry, tea_var_t, link);
    rtl_list_remove(entry);
    return variable;
  }
#endif
  return rtl_malloc(sizeof(tea_var_t));
}

void tea_free_var(tea_ctx_t* context, tea_var_t* variable)
{
#if TEA_VARIABLE_POOL_ENABLED
  if (variable) {
    rtl_list_add_head(&context->vars, &variable->link);
  }
#else
  rtl_free(variable);
#endif
}

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent)
{
  scope->parent = parent;
  rtl_list_init(&scope->vars);
}

void tea_scope_cleanup(tea_ctx_t* context, const tea_scope_t* scope)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &scope->vars)
  {
    tea_var_t* variable = rtl_list_record(entry, tea_var_t, link);
    rtl_list_remove(entry);
    tea_free_var(context, variable);
  }
}

tea_var_t* tea_scope_find_local(const tea_scope_t* scope, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &scope->vars)
  {
    tea_var_t* variable = rtl_list_record(entry, tea_var_t, link);
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

tea_var_t* tea_scope_find(const tea_scope_t* scope, const char* name)
{
  const tea_scope_t* current_scope = scope;
  while (current_scope) {
    tea_var_t* variable = tea_scope_find_local(current_scope, name);
    if (variable) {
      return variable;
    }

    current_scope = current_scope->parent;
  }

  return NULL;
}

bool tea_decl_var(tea_ctx_t* context, tea_scope_t* scope, const char* name,
  const unsigned int flags, const char* type, const tea_node_t* initial_value)
{
  tea_var_t* variable = tea_scope_find_local(scope, name);
  if (variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' is already declared in current scope - redeclaration not "
      "allowed",
      variable->name);
    return false;
  }

  variable = tea_alloc_var(context);
  if (!variable) {
    rtl_log_err("Memory error: Failed to allocate memory for variable '%s'", name);
    return false;
  }

  variable->name = name;
  variable->flags = flags;
  tea_val_t value = tea_eval_expr(context, scope, initial_value);
  if (value.type == TEA_V_UNDEF) {
    tea_free_var(context, variable);
    return false;
  }
  if (type) {
    const tea_val_type_t predefined_type = tea_value_get_type_by_string(type);
    if (predefined_type == TEA_V_UNDEF) {
      rtl_log_err("Runtime error: Unknown type '%s' specified in variable declaration", type);
      tea_free_var(context, variable);
      return false;
    }
    if (value.type == TEA_V_NULL) {
      value.null_type = predefined_type;
    } else if (value.type != predefined_type) {
      rtl_log_err(
        "Runtime error: Type mismatch - value type does not match declared variable type");
      tea_free_var(context, variable);
      return false;
    }
  }
  variable->val = value;

  switch (variable->val.type) {
    case TEA_V_I32:
      rtl_log_dbg("Declare variable %s : %s = %d", name, tea_val_type_str(variable->val.type),
        variable->val.i32);
      break;
    case TEA_V_F32:
      rtl_log_dbg("Declare variable %s : %s = %f", name, tea_val_type_str(variable->val.type),
        variable->val.f32);
      break;
    case TEA_V_NULL:
      rtl_log_dbg("Declare variable %s : %s = null", name, tea_val_type_str(variable->val.type));
    default:
      break;
  }

  rtl_list_add_tail(&scope->vars, &variable->link);

  return true;
}

#undef TEA_VARIABLE_POOL_ENABLED
