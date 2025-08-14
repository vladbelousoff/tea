#include "tea_scope.h"
#include "tea_expr.h"

#include <stdlib.h>
#include <string.h>

#include "tea.h"
#include "tea_log.h"
#include "tea_memory.h"

#define TEA_VARIABLE_POOL_ENABLED 1

tea_var_t *tea_alloc_var(const tea_ctx_t *ctx)
{
#if TEA_VARIABLE_POOL_ENABLED
  tea_list_entry_t *entry;
  tea_list_entry_t *safe;

  tea_list_for_each_safe(entry, safe, &ctx->vars)
  {
    tea_var_t *variable = tea_list_record(entry, tea_var_t, link);
    tea_list_remove(entry);
    return variable;
  }
#endif
  return tea_malloc(sizeof(tea_var_t));
}

void tea_free_var(tea_ctx_t *ctx, tea_var_t *variable)
{
#if TEA_VARIABLE_POOL_ENABLED
  if (variable) {
    tea_list_add_head(&ctx->vars, &variable->link);
  }
#else
  tea_free(variable);
#endif
}

void tea_scope_init(tea_scope_t *scp, tea_scope_t *parent)
{
  scp->parent = parent;
  tea_list_init(&scp->vars);
}

void tea_scope_cleanup(tea_ctx_t *ctx, const tea_scope_t *scp)
{
  tea_list_entry_t *entry;
  tea_list_entry_t *safe;

  tea_list_for_each_safe(entry, safe, &scp->vars)
  {
    tea_var_t *variable = tea_list_record(entry, tea_var_t, link);
    tea_list_remove(entry);
    tea_free_var(ctx, variable);
  }
}

tea_var_t *tea_scope_find_local(const tea_scope_t *scp, const char *name)
{
  tea_list_entry_t *entry;
  tea_list_for_each(entry, &scp->vars)
  {
    tea_var_t *variable = tea_list_record(entry, tea_var_t, link);
    const char *variable_name = variable->name;
    if (!variable_name) {
      continue;
    }
    if (!strcmp(variable_name, name)) {
      return variable;
    }
  }

  return NULL;
}

tea_var_t *tea_scope_find(const tea_scope_t *scp, const char *name)
{
  const tea_scope_t *current_scope = scp;
  while (current_scope) {
    tea_var_t *variable = tea_scope_find_local(current_scope, name);
    if (variable) {
      return variable;
    }

    current_scope = current_scope->parent;
  }

  return NULL;
}

bool tea_decl_var(tea_ctx_t *ctx, tea_scope_t *scp, const char *name,
                  const unsigned int flags, const char *type,
                  const tea_node_t *initial_value)
{
  tea_var_t *variable = tea_scope_find_local(scp, name);
  if (variable) {
    tea_log_err(
      "Runtime error: Variable '%s' is already declared in current scope - redeclaration not "
      "allowed",
      variable->name);
    return false;
  }

  variable = tea_alloc_var(ctx);
  if (!variable) {
    tea_log_err("Memory error: Failed to allocate memory for variable '%s'",
                name);
    return false;
  }

  variable->name = name;
  variable->flags = flags;
  tea_val_t value = tea_eval_expr(ctx, scp, initial_value);
  if (value.type == TEA_V_UNDEF) {
    tea_free_var(ctx, variable);
    return false;
  }
  if (type) {
    const tea_val_type_t predefined_type = tea_val_type_by_str(type);
    if (predefined_type == TEA_V_UNDEF) {
      tea_log_err(
        "Runtime error: Unknown type '%s' specified in variable declaration",
        type);
      tea_free_var(ctx, variable);
      return false;
    }
    if (value.type == TEA_V_NULL) {
      value.null_type = predefined_type;
    } else if (value.type != predefined_type) {
      tea_log_err(
        "Runtime error: Type mismatch - value type does not match declared variable type");
      tea_free_var(ctx, variable);
      return false;
    }
  }
  variable->val = value;

  switch (variable->val.type) {
  case TEA_V_I32:
    tea_log_dbg("Declare variable %s : %s = %d", name,
                tea_val_type_str(variable->val.type), variable->val.i32);
    break;
  case TEA_V_F32:
    tea_log_dbg("Declare variable %s : %s = %f", name,
                tea_val_type_str(variable->val.type), variable->val.f32);
    break;
  case TEA_V_NULL:
    tea_log_dbg("Declare variable %s : %s = null", name,
                tea_val_type_str(variable->val.type));
  default:
    break;
  }

  tea_list_add_tail(&scp->vars, &variable->link);

  return true;
}

#undef TEA_VARIABLE_POOL_ENABLED
