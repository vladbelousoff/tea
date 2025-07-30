#include "tea_statement.h"
#include "tea_expression.h"
#include "tea_function.h"
#include "tea_struct.h"

#include <stdlib.h>

#include <rtl.h>
#include <rtl_log.h>

bool tea_interpret_execute_stmt(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context,
  tea_loop_context_t* loop_context)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    if (!tea_interpret_execute(context, scope, child, return_context, loop_context)) {
      return false;
    }
  }

  return true;
}

bool tea_interpret_let(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* name = node->token;

  unsigned int flags = 0;
  const tea_ast_node_t* type = NULL;
  const tea_ast_node_t* expr = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_MUT:
        flags |= TEA_VARIABLE_MUTABLE;
        break;
      case TEA_AST_NODE_OPTIONAL:
        flags |= TEA_VARIABLE_OPTIONAL;
        break;
      case TEA_AST_NODE_TYPE_ANNOT:
        type = child;
        break;
      default:
        expr = child;
        break;
    }
  }

  const tea_token_t* type_token = type ? type->token : NULL;
  const char* type_name = type_token ? type_token->buffer : NULL;
  return tea_declare_variable(context, scope, name->buffer, flags, type_name, expr);
}

bool tea_interpret_assign(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* name = node->token;
  if (!name) {
    // Probably if the name is null, then it's a field access
    rtl_list_entry_t* field_entry = rtl_list_first(&node->children);
    if (!field_entry) {
      exit(1);
    }

    const tea_ast_node_t* field_node = rtl_list_record(field_entry, tea_ast_node_t, link);
    if (!field_node) {
      exit(1);
    }

    tea_value_t* value = tea_get_field_pointer(context, scope, field_node);
    if (!value) {
      return false;
    }

    // First child is the value on the right
    const tea_ast_node_t* rhs =
      rtl_list_record(rtl_list_first(&node->children), tea_ast_node_t, link);
    const tea_value_t new_value = tea_interpret_evaluate_expression(context, scope, rhs);

    // TODO: Make it clearer with checking mutability
    *value = new_value;

    return true;
  }

  tea_variable_t* variable = tea_scope_find_variable(scope, name->buffer);
  if (!variable) {
    rtl_log_err("Cannot find variable '%s'", name->buffer);
    exit(1);
  }

  if (!(variable->flags & TEA_VARIABLE_MUTABLE)) {
    rtl_log_err("Variable '%s' is not mutable, so cannot be modified", name->buffer);
    exit(1);
  }

  // First child is the value on the right
  const tea_ast_node_t* rhs =
    rtl_list_record(rtl_list_first(&node->children), tea_ast_node_t, link);
  const tea_value_t new_value = tea_interpret_evaluate_expression(context, scope, rhs);

  if (new_value.type == variable->value.type) {
    variable->value = new_value;
  } else {
    rtl_log_err(
      "Cannot assign variable '%s', because the original type is %s, but the new type is %s",
      name->buffer, tea_value_get_type_string(variable->value.type),
      tea_value_get_type_string(new_value.type));
    exit(1);
  }

  switch (variable->value.type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("New value for variable %s : %s = %d", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.i32);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("New value for variable %s : %s = %f", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.f32);
      break;
    case TEA_VALUE_STRING:
      rtl_log_dbg("New value for variable %s : %s = '%s'", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.string);
      break;
    default:
      break;
  }

  return true;
}

bool tea_interpret_execute_if(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context,
  tea_loop_context_t* loop_context)
{
  const tea_ast_node_t* condition = NULL;
  const tea_ast_node_t* then_node = NULL;
  const tea_ast_node_t* else_node = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_THEN:
        then_node = child;
        break;
      case TEA_AST_NODE_ELSE:
        else_node = child;
        break;
      default:
        condition = child;
        break;
    }
  }

  tea_scope_t inner_scope;
  tea_scope_init(&inner_scope, scope);

  const tea_value_t cond_val = tea_interpret_evaluate_expression(context, &inner_scope, condition);

  bool result = true;
  if (cond_val.i32 != 0) {
    result = tea_interpret_execute(context, &inner_scope, then_node, return_context, loop_context);
  } else if (else_node) {
    result = tea_interpret_execute(context, &inner_scope, else_node, return_context, loop_context);
  }

  tea_scope_cleanup(context, &inner_scope);
  return result;
}

bool tea_interpret_execute_while(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context)
{
  const tea_ast_node_t* while_cond = NULL;
  const tea_ast_node_t* while_body = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_WHILE_COND:
        while_cond = rtl_list_record(rtl_list_first(&child->children), tea_ast_node_t, link);
        break;
      case TEA_AST_NODE_WHILE_BODY:
        while_body = child;
        break;
      default:
        break;
    }
  }

  if (!while_cond) {
    return false;
  }

  tea_loop_context_t loop_context;
  loop_context.is_break_set = false;
  loop_context.is_continue_set = false;

  while (true) {
    const tea_value_t cond_val = tea_interpret_evaluate_expression(context, scope, while_cond);
    if (cond_val.i32 == 0) {
      break;
    }

    // Reset continue flag at the beginning of each iteration
    loop_context.is_continue_set = false;

    tea_scope_t inner_scope;
    tea_scope_init(&inner_scope, scope);
    const bool result =
      tea_interpret_execute(context, &inner_scope, while_body, return_context, &loop_context);
    tea_scope_cleanup(context, &inner_scope);

    if (!result) {
      return false;
    }

    if (return_context && return_context->is_set) {
      break;
    }

    if (loop_context.is_break_set) {
      break;
    }
  }

  return true;
}

bool tea_interpret_execute_return(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context)
{
  rtl_list_entry_t* entry = rtl_list_first(&node->children);
  if (entry) {
    const tea_ast_node_t* expr = rtl_list_record(entry, tea_ast_node_t, link);
    if (expr && return_context) {
      return_context->returned_value = tea_interpret_evaluate_expression(context, scope, expr);
      return_context->is_set = true;
    }
  }

  return true;
}

bool tea_interpret_execute_break(tea_loop_context_t* loop_context)
{
  if (loop_context) {
    loop_context->is_break_set = true;
    return true;
  }

  rtl_log_err("Break cannot be used outside of loops");
  return false;
}

bool tea_interpret_execute_continue(tea_loop_context_t* loop_context)
{
  if (loop_context) {
    loop_context->is_continue_set = true;
    return true;
  }

  rtl_log_err("Continue cannot be used outside of loops");
  return false;
}

bool tea_interpret_execute(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node,
  tea_return_context_t* return_context, tea_loop_context_t* loop_context)
{
  if (return_context && return_context->is_set) {
    return true;
  }

  if (loop_context && loop_context->is_break_set) {
    return true;
  }

  if (loop_context && loop_context->is_continue_set) {
    return true;
  }

  switch (node->type) {
    case TEA_AST_NODE_LET:
      return tea_interpret_let(context, scope, node);
    case TEA_AST_NODE_ASSIGN:
      return tea_interpret_assign(context, scope, node);
    case TEA_AST_NODE_IF:
      return tea_interpret_execute_if(context, scope, node, return_context, loop_context);
    case TEA_AST_NODE_WHILE:
      return tea_interpret_execute_while(context, scope, node, return_context);
    case TEA_AST_NODE_FUNCTION:
      return tea_interpret_function_declaration(context, node);
    case TEA_AST_NODE_RETURN:
      return tea_interpret_execute_return(context, scope, node, return_context);
    case TEA_AST_NODE_BREAK:
      return tea_interpret_execute_break(loop_context);
    case TEA_AST_NODE_CONTINUE:
      return tea_interpret_execute_continue(loop_context);
    case TEA_AST_NODE_FUNCTION_CALL:
      tea_interpret_evaluate_function_call(context, scope, node);
      return true;
    case TEA_AST_NODE_STRUCT:
      return tea_interpret_struct_declaration(context, node);
    case TEA_AST_NODE_IMPL_BLOCK:
      return tea_interpret_impl_block(context, node);
    case TEA_AST_NODE_PROGRAM:
    case TEA_AST_NODE_STMT:
    case TEA_AST_NODE_FUNCTION_CALL_ARGS:
    case TEA_AST_NODE_THEN:
    case TEA_AST_NODE_ELSE:
    case TEA_AST_NODE_WHILE_COND:
    case TEA_AST_NODE_WHILE_BODY:
      return tea_interpret_execute_stmt(context, scope, node, return_context, loop_context);
    default: {
      const tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Not implemented node <%s> in file %s, token: <%s> '%.*s' (line %d, col %d)",
          tea_ast_node_get_type_name(node->type), context->filename,
          tea_token_get_name(token->type), token->buffer_size, token->buffer, token->line,
          token->column);
      } else {
        rtl_log_err("Not implemented node <%s> in file %s", tea_ast_node_get_type_name(node->type),
          context->filename);
      }
    } break;
  }

  return false;
}
