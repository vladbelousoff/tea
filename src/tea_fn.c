#include "tea_fn.h"

#include "tea_expr.h"
#include "tea_stmt.h"
#include "tea_struct.h"
#include "tea_trait.h"

#include <stdlib.h>
#include <string.h>

#include <rtl_log.h>
#include <rtl_memory.h>

#include "tea_grammar.h"

tea_var_t* tea_fn_args_pop(const tea_fn_args_t* args)
{
  rtl_list_entry_t* first = rtl_list_first(&args->head);
  if (first) {
    tea_var_t* arg = rtl_list_record(first, tea_var_t, link);
    rtl_list_remove(first);
    return arg;
  }

  return NULL;
}

const tea_native_fn_t* tea_ctx_find_native_fn(const rtl_list_entry_t* functions, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, functions)
  {
    const tea_native_fn_t* function = rtl_list_record(entry, tea_native_fn_t, link);
    if (!strcmp(function->name, name)) {
      return function;
    }
  }

  return NULL;
}

const tea_fn_t* tea_ctx_find_fn(const rtl_list_entry_t* functions, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, functions)
  {
    const tea_fn_t* function = rtl_list_record(entry, tea_fn_t, link);
    const tea_tok_t* function_name = function->name;
    if (!function_name) {
      continue;
    }
    if (!strcmp(function_name->buf, name)) {
      return function;
    }
  }

  return NULL;
}

bool tea_declare_function(const tea_node_t* node, rtl_list_entry_t* functions)
{
  const tea_tok_t* fn_name = node->tok;
  if (!fn_name) {
    return false;
  }

  const tea_node_t* fn_ret_type = NULL;
  const tea_node_t* fn_body = NULL;
  const tea_node_t* fn_params = NULL;

  bool is_mutable = false;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    switch (child->type) {
      case TEA_N_PARAM:
        fn_params = child;
        break;
      case TEA_N_RET_TYPE:
        fn_ret_type = child;
        break;
      case TEA_TOKEN_MUT:
        is_mutable = true;
        break;
      default:
        fn_body = child;
        break;
    }
  }

  tea_fn_t* fn = rtl_malloc(sizeof(*fn));
  if (!fn) {
    return false;
  }

  fn->name = fn_name;
  fn->body = fn_body;
  fn->mut = is_mutable;
  fn->params = fn_params;
  if (fn_ret_type) {
    fn->ret_type = fn_ret_type->tok;
  } else {
    fn->ret_type = NULL;
  }

  rtl_list_add_tail(functions, &fn->link);

  if (fn->ret_type) {
    rtl_log_dbg("Declare function: '%.*s' -> %.*s", fn_name->size, fn_name->buf, fn->ret_type->size,
      fn->ret_type->buf);
  } else {
    rtl_log_dbg("Declare function: '%.*s'", fn_name->size, fn_name->buf);
  }

  return true;
}

bool tea_interpret_function_declaration(tea_ctx_t* context, const tea_node_t* node)
{
  return tea_declare_function(node, &context->fns);
}

tea_val_t tea_interpret_evaluate_native_function_call(tea_ctx_t* context, tea_scope_t* scope,
  const tea_native_fn_t* native_function, const tea_node_t* function_call_args)
{
  tea_fn_args_t function_args;
  rtl_list_init(&function_args.head);

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &function_call_args->children)
  {
    const tea_node_t* arg_expr = rtl_list_record(entry, tea_node_t, link);
    tea_var_t* arg = tea_alloc_var(context);
    if (!arg) {
      rtl_log_err("Memory error: Failed to allocate variable for function argument");
      // Clean up any previously allocated args
      rtl_list_entry_t* cleanup_safe;
      rtl_list_for_each_safe(entry, cleanup_safe, &function_args.head)
      {
        tea_var_t* cleanup_arg = rtl_list_record(entry, tea_var_t, link);
        rtl_list_remove(entry);
        tea_free_var(context, cleanup_arg);
      }
      return tea_val_undef();
    }
    arg->val = tea_eval_expr(context, scope, arg_expr);
    if (arg->val.type == TEA_V_UNDEF) {
      tea_free_var(context, arg);
      // Clean up any previously allocated args
      rtl_list_entry_t* cleanup_safe;
      rtl_list_for_each_safe(entry, cleanup_safe, &function_args.head)
      {
        tea_var_t* cleanup_arg = rtl_list_record(entry, tea_var_t, link);
        rtl_list_remove(entry);
        tea_free_var(context, cleanup_arg);
      }
      return tea_val_undef();
    }
    // TODO: Check mutability and optionality
    arg->flags = 0;
    // TODO: Set the proper arg name
    arg->name = "unknown";

    rtl_list_add_tail(&function_args.head, &arg->link);
  }

  const tea_val_t result = native_function->cb(context, &function_args);

  // Deallocate all the args
  rtl_list_entry_t* safe;
  rtl_list_for_each_safe(entry, safe, &function_args.head)
  {
    tea_var_t* arg = rtl_list_record(entry, tea_var_t, link);
    rtl_list_remove(entry);
    tea_free_var(context, arg);
  }

  return result;
}

tea_val_t tea_eval_fn_call(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node)
{
  const tea_node_t* function_call_args = NULL;
  const tea_node_t* field_access = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    switch (child->type) {
      case TEA_N_FN_ARGS:
        function_call_args = child;
        break;
      case TEA_N_FIELD_ACC:
        field_access = child;
      default:
        break;
    }
  }

  const tea_fn_t* function = NULL;
  const char* function_name = NULL;

  tea_ret_ctx_t return_context = { 0 };
  return_context.is_set = false;

  tea_scope_t inner_scope;
  tea_scope_init(&inner_scope, scope);

  if (field_access) {
    const tea_node_t* object_node = field_access->field_acc.obj;
    const tea_node_t* field_node = field_access->field_acc.field;
    if (!field_node || !object_node) {
      rtl_log_err(
        "Internal error: Missing field or object node in method call - AST structure corrupted");
      tea_scope_cleanup(context, &inner_scope);
      return tea_val_undef();
    }

    const tea_tok_t* object_token = object_node->tok;
    const tea_tok_t* field_token = field_node->tok;
    if (!field_token || !object_token) {
      rtl_log_err(
        "Internal error: Missing field or object token in method call - AST structure corrupted");
      tea_scope_cleanup(context, &inner_scope);
      return tea_val_undef();
    }

    tea_var_t* variable = tea_scope_find(scope, object_token->buf);
    if (!variable) {
      rtl_log_err(
        "Runtime error: Undefined variable '%s' used in method call at line %d, column %d",
        object_token->buf, object_token->line, object_token->col);
      tea_scope_cleanup(context, &inner_scope);
      return tea_val_undef();
    }

    if (variable->val.type != TEA_V_INST) {
      rtl_log_err(
        "Runtime error: Methods can only be called on object instances, not on primitive types");
      tea_scope_cleanup(context, &inner_scope);
      return tea_val_undef();
    }

    tea_struct_decl_t* struct_declaration = tea_find_struct_decl(context, variable->val.obj->type);
    if (!struct_declaration) {
      rtl_log_err("Runtime error: Cannot find struct declaration for type '%s' when calling method",
        variable->val.obj->type);
      tea_scope_cleanup(context, &inner_scope);
      return tea_val_undef();
    }

    // declare 'self' for the scope
    tea_decl_var(context, &inner_scope, "self", 0, NULL, object_node);

    function_name = field_token->buf;
    function = tea_ctx_find_fn(&struct_declaration->fns, function_name);

    // If not found in struct methods, try trait methods
    if (!function) {
      function = tea_resolve_trait_method(context, variable->val.obj->type, function_name);
    }
  } else {
    const tea_tok_t* token = node->tok;
    if (token) {
      const tea_native_fn_t* native_function = tea_ctx_find_native_fn(&context->nfns, token->buf);
      if (native_function) {
        return tea_interpret_evaluate_native_function_call(
          context, scope, native_function, function_call_args);
      }

      function_name = token->buf;
      function = tea_ctx_find_fn(&context->fns, token->buf);
    }
  }

  if (!function) {
    if (field_access) {
      const tea_tok_t* field_token = field_access->field_acc.field->tok;
      rtl_log_err("Runtime error: Undefined method '%s' called at line %d, column %d",
        function_name, field_token->line, field_token->col);
    } else {
      const tea_tok_t* token = node->tok;
      if (token) {
        rtl_log_err("Runtime error: Undefined function '%s' called at line %d, column %d",
          function_name, token->line, token->col);
      } else {
        rtl_log_err(
          "Runtime error: Undefined function '%s' called (no position information available)",
          function_name);
      }
    }

    return tea_val_undef();
  }

  const tea_node_t* function_params = function->params;
  if (function_params && function_call_args) {
    rtl_list_entry_t* param_name_entry = rtl_list_first(&function_params->children);
    rtl_list_entry_t* param_expr_entry = rtl_list_first(&function_call_args->children);

    while (param_name_entry && param_expr_entry) {
      const tea_node_t* param_name = rtl_list_record(param_name_entry, tea_node_t, link);
      const tea_node_t* param_expr = rtl_list_record(param_expr_entry, tea_node_t, link);

      if (!param_name) {
        break;
      }

      if (!param_expr) {
        break;
      }

      tea_tok_t* param_name_token = param_name->tok;
      if (!param_name_token) {
        break;
      }

      tea_var_t* variable = tea_alloc_var(context);
      if (!variable) {
        rtl_log_err(
          "Memory error: Failed to allocate memory for function parameter '%.*s' at line %d, "
          "column %d",
          param_name_token->size, param_name_token->buf, param_name_token->line,
          param_name_token->col);
        exit(1);
      }

      variable->name = param_name_token->buf;
      /* TODO: Currently all function arguments are not mutable by default and I don't check the
       * types */
      variable->flags = 0;
      variable->val = tea_eval_expr(context, scope, param_expr);

      switch (variable->val.type) {
        case TEA_V_I32:
          rtl_log_dbg("Declare param %s : %s = %d", param_name_token->buf,
            tea_val_type_str(variable->val.type), variable->val.i32);
          break;
        case TEA_V_F32:
          rtl_log_dbg("Declare param %s : %s = %f", param_name_token->buf,
            tea_val_type_str(variable->val.type), variable->val.f32);
          break;
        default:
          break;
      }

      // TODO: Check if the param already exists
      rtl_list_add_tail(&inner_scope.vars, &variable->link);

      param_name_entry = rtl_list_next(param_name_entry, &function_params->children);
      param_expr_entry = rtl_list_next(param_expr_entry, &function_call_args->children);
    }
  }

  const bool result = tea_exec(context, &inner_scope, function->body, &return_context, NULL);
  tea_scope_cleanup(context, &inner_scope);

  if (result && return_context.is_set) {
    return return_context.ret_val;
  }

  return tea_val_undef();
}

void tea_bind_native_fn(tea_ctx_t* ctx, const char* name, const tea_native_fn_cb_t cb)
{
  tea_native_fn_t* function = rtl_malloc(sizeof(*function));
  if (function) {
    function->name = name;
    function->cb = cb;
    rtl_list_add_tail(&ctx->nfns, &function->link);
  }
}
