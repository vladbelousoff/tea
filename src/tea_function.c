#include "tea_function.h"

#include "tea_expression.h"
#include "tea_statement.h"
#include "tea_struct.h"
#include "tea_trait.h"

#include <stdlib.h>
#include <string.h>

#include <rtl_log.h>
#include <rtl_memory.h>

#include "tea_grammar.h"

tea_variable_t* tea_function_args_pop(const tea_function_args_t* args)
{
  rtl_list_entry_t* first = rtl_list_first(&args->list_head);
  if (first) {
    tea_variable_t* arg = rtl_list_record(first, tea_variable_t, link);
    rtl_list_remove(first);
    return arg;
  }

  return NULL;
}

const tea_native_function_t* tea_context_find_native_function(
  const rtl_list_entry_t* functions, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, functions)
  {
    const tea_native_function_t* function = rtl_list_record(entry, tea_native_function_t, link);
    if (!strcmp(function->name, name)) {
      return function;
    }
  }

  return NULL;
}

const tea_function_t* tea_context_find_function(const rtl_list_entry_t* functions, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, functions)
  {
    const tea_function_t* function = rtl_list_record(entry, tea_function_t, link);
    const tea_token_t* function_name = function->name;
    if (!function_name) {
      continue;
    }
    if (!strcmp(function_name->buffer, name)) {
      return function;
    }
  }

  return NULL;
}

bool tea_declare_function(const tea_ast_node_t* node, rtl_list_entry_t* functions)
{
  const tea_token_t* fn_name = node->token;
  if (!fn_name) {
    return false;
  }

  const tea_ast_node_t* fn_return_type = NULL;
  const tea_ast_node_t* fn_body = NULL;
  const tea_ast_node_t* fn_params = NULL;

  bool is_mutable = false;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_PARAM:
        fn_params = child;
        break;
      case TEA_AST_NODE_RETURN_TYPE:
        fn_return_type = child;
        break;
      case TEA_TOKEN_MUT:
        is_mutable = true;
        break;
      default:
        fn_body = child;
        break;
    }
  }

  tea_function_t* fn = rtl_malloc(sizeof(*fn));
  if (!fn) {
    return false;
  }

  fn->name = fn_name;
  fn->body = fn_body;
  fn->mut = is_mutable;
  fn->params = fn_params;
  if (fn_return_type) {
    fn->return_type = fn_return_type->token;
  } else {
    fn->return_type = NULL;
  }

  rtl_list_add_tail(functions, &fn->link);

  if (fn->return_type) {
    rtl_log_dbg("Declare function: '%.*s' -> %.*s", fn_name->buffer_size, fn_name->buffer,
      fn->return_type->buffer_size, fn->return_type->buffer);
  } else {
    rtl_log_dbg("Declare function: '%.*s'", fn_name->buffer_size, fn_name->buffer);
  }

  return true;
}

bool tea_interpret_function_declaration(tea_context_t* context, const tea_ast_node_t* node)
{
  return tea_declare_function(node, &context->functions);
}

tea_value_t tea_interpret_evaluate_native_function_call(tea_context_t* context, tea_scope_t* scope,
  const tea_native_function_t* native_function, const tea_ast_node_t* function_call_args)
{
  tea_function_args_t function_args;
  rtl_list_init(&function_args.list_head);

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &function_call_args->children)
  {
    const tea_ast_node_t* arg_expr = rtl_list_record(entry, tea_ast_node_t, link);
    tea_variable_t* arg = tea_allocate_variable(context);
    if (!arg) {
      rtl_log_err("Memory error: Failed to allocate variable for function argument");
      // Clean up any previously allocated args
      rtl_list_entry_t* cleanup_safe;
      rtl_list_for_each_safe(entry, cleanup_safe, &function_args.list_head)
      {
        tea_variable_t* cleanup_arg = rtl_list_record(entry, tea_variable_t, link);
        rtl_list_remove(entry);
        tea_free_variable(context, cleanup_arg);
      }
      return tea_value_invalid();
    }
    arg->value = tea_interpret_evaluate_expression(context, scope, arg_expr);
    if (arg->value.type == TEA_VALUE_INVALID) {
      tea_free_variable(context, arg);
      // Clean up any previously allocated args
      rtl_list_entry_t* cleanup_safe;
      rtl_list_for_each_safe(entry, cleanup_safe, &function_args.list_head)
      {
        tea_variable_t* cleanup_arg = rtl_list_record(entry, tea_variable_t, link);
        rtl_list_remove(entry);
        tea_free_variable(context, cleanup_arg);
      }
      return tea_value_invalid();
    }
    // TODO: Check mutability and optionality
    arg->flags = 0;
    // TODO: Set the proper arg name
    arg->name = "unknown";

    rtl_list_add_tail(&function_args.list_head, &arg->link);
  }

  const tea_value_t result = native_function->cb(context, &function_args);

  // Deallocate all the args
  rtl_list_entry_t* safe;
  rtl_list_for_each_safe(entry, safe, &function_args.list_head)
  {
    tea_variable_t* arg = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    tea_free_variable(context, arg);
  }

  return result;
}

tea_value_t tea_interpret_evaluate_function_call(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_ast_node_t* function_call_args = NULL;
  const tea_ast_node_t* field_access = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_FUNCTION_CALL_ARGS:
        function_call_args = child;
        break;
      case TEA_AST_NODE_FIELD_ACCESS:
        field_access = child;
      default:
        break;
    }
  }

  const tea_function_t* function = NULL;
  const char* function_name = NULL;

  tea_return_context_t return_context = { 0 };
  return_context.is_set = false;

  tea_scope_t inner_scope;
  tea_scope_init(&inner_scope, scope);

  if (field_access) {
    const tea_ast_node_t* object_node = field_access->field_access.object;
    const tea_ast_node_t* field_node = field_access->field_access.field;
    if (!field_node || !object_node) {
      rtl_log_err(
        "Internal error: Missing field or object node in method call - AST structure corrupted");
      tea_scope_cleanup(context, &inner_scope);
      return tea_value_invalid();
    }

    const tea_token_t* object_token = object_node->token;
    const tea_token_t* field_token = field_node->token;
    if (!field_token || !object_token) {
      rtl_log_err(
        "Internal error: Missing field or object token in method call - AST structure corrupted");
      tea_scope_cleanup(context, &inner_scope);
      return tea_value_invalid();
    }

    tea_variable_t* variable = tea_scope_find_variable(scope, object_token->buffer);
    if (!variable) {
      rtl_log_err(
        "Runtime error: Undefined variable '%s' used in method call at line %d, column %d",
        object_token->buffer, object_token->line, object_token->column);
      tea_scope_cleanup(context, &inner_scope);
      return tea_value_invalid();
    }

    if (variable->value.type != TEA_VALUE_INSTANCE) {
      rtl_log_err(
        "Runtime error: Methods can only be called on object instances, not on primitive types");
      tea_scope_cleanup(context, &inner_scope);
      return tea_value_invalid();
    }

    tea_struct_declaration_t* struct_declaration =
      tea_find_struct_declaration(context, variable->value.object->type);
    if (!struct_declaration) {
      rtl_log_err("Runtime error: Cannot find struct declaration for type '%s' when calling method",
        variable->value.object->type);
      tea_scope_cleanup(context, &inner_scope);
      return tea_value_invalid();
    }

    // declare 'self' for the scope
    tea_declare_variable(context, &inner_scope, "self", 0, NULL, object_node);

    function_name = field_token->buffer;
    function = tea_context_find_function(&struct_declaration->functions, function_name);

    // If not found in struct methods, try trait methods
    if (!function) {
      function = tea_resolve_trait_method(context, variable->value.object->type, function_name);
    }
  } else {
    const tea_token_t* token = node->token;
    if (token) {
      const tea_native_function_t* native_function =
        tea_context_find_native_function(&context->native_functions, token->buffer);
      if (native_function) {
        return tea_interpret_evaluate_native_function_call(
          context, scope, native_function, function_call_args);
      }

      function_name = token->buffer;
      function = tea_context_find_function(&context->functions, token->buffer);
    }
  }

  if (!function) {
    if (field_access) {
      const tea_token_t* field_token = field_access->field_access.field->token;
      rtl_log_err("Runtime error: Undefined method '%s' called at line %d, column %d",
        function_name, field_token->line, field_token->column);
    } else {
      const tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Runtime error: Undefined function '%s' called at line %d, column %d",
          function_name, token->line, token->column);
      } else {
        rtl_log_err(
          "Runtime error: Undefined function '%s' called (no position information available)",
          function_name);
      }
    }

    return tea_value_invalid();
  }

  const tea_ast_node_t* function_params = function->params;
  if (function_params && function_call_args) {
    rtl_list_entry_t* param_name_entry = rtl_list_first(&function_params->children);
    rtl_list_entry_t* param_expr_entry = rtl_list_first(&function_call_args->children);

    while (param_name_entry && param_expr_entry) {
      const tea_ast_node_t* param_name = rtl_list_record(param_name_entry, tea_ast_node_t, link);
      const tea_ast_node_t* param_expr = rtl_list_record(param_expr_entry, tea_ast_node_t, link);

      if (!param_name) {
        break;
      }

      if (!param_expr) {
        break;
      }

      tea_token_t* param_name_token = param_name->token;
      if (!param_name_token) {
        break;
      }

      tea_variable_t* variable = tea_allocate_variable(context);
      if (!variable) {
        rtl_log_err(
          "Memory error: Failed to allocate memory for function parameter '%.*s' at line %d, "
          "column %d",
          param_name_token->buffer_size, param_name_token->buffer, param_name_token->line,
          param_name_token->column);
        exit(1);
      }

      variable->name = param_name_token->buffer;
      /* TODO: Currently all function arguments are not mutable by default and I don't check the
       * types */
      variable->flags = 0;
      variable->value = tea_interpret_evaluate_expression(context, scope, param_expr);

      switch (variable->value.type) {
        case TEA_VALUE_I32:
          rtl_log_dbg("Declare param %s : %s = %d", param_name_token->buffer,
            tea_value_get_type_string(variable->value.type), variable->value.i32);
          break;
        case TEA_VALUE_F32:
          rtl_log_dbg("Declare param %s : %s = %f", param_name_token->buffer,
            tea_value_get_type_string(variable->value.type), variable->value.f32);
          break;
        default:
          break;
      }

      // TODO: Check if the param already exists
      rtl_list_add_tail(&inner_scope.variables, &variable->link);

      param_name_entry = rtl_list_next(param_name_entry, &function_params->children);
      param_expr_entry = rtl_list_next(param_expr_entry, &function_call_args->children);
    }
  }

  const bool result =
    tea_interpret_execute(context, &inner_scope, function->body, &return_context, NULL);
  tea_scope_cleanup(context, &inner_scope);

  if (result && return_context.is_set) {
    return return_context.returned_value;
  }

  return tea_value_invalid();
}

void tea_bind_native_function(
  tea_context_t* context, const char* name, const tea_native_function_cb_t cb)
{
  tea_native_function_t* function = rtl_malloc(sizeof(*function));
  if (function) {
    function->name = name;
    function->cb = cb;
    rtl_list_add_tail(&context->native_functions, &function->link);
  }
}
