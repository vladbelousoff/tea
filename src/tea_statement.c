#include "tea_statement.h"
#include "tea_expression.h"
#include "tea_function.h"
#include "tea_struct.h"
#include "tea_trait.h"

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

static void tea_extract_type_info(
  const tea_ast_node_t* type_annot, const char** type_name, bool* is_optional)
{
  *type_name = NULL;
  *is_optional = false;

  if (!type_annot || type_annot->type != TEA_AST_NODE_TYPE_ANNOT) {
    return;
  }

  rtl_list_entry_t* entry = rtl_list_first(&type_annot->children);
  if (!entry) {
    return;
  }

  const tea_ast_node_t* type_spec = rtl_list_record(entry, tea_ast_node_t, link);

  if (type_spec->token) {
    *type_name = type_spec->token->buffer;
  }

  if (type_spec->type == TEA_AST_NODE_OPTIONAL_TYPE) {
    *is_optional = true;
  }
}

bool tea_interpret_let(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* name = node->token;

  unsigned int flags = 0;
  const tea_ast_node_t* type_annot = NULL;
  const tea_ast_node_t* expr = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_MUT:
        flags |= TEA_VARIABLE_MUTABLE;
        break;
      case TEA_AST_NODE_TYPE_ANNOT:
        type_annot = child;
        break;
      default:
        expr = child;
        break;
    }
  }

  const char* type_name;
  bool is_optional;
  tea_extract_type_info(type_annot, &type_name, &is_optional);

  if (is_optional) {
    flags |= TEA_VARIABLE_OPTIONAL;
  }

  return tea_declare_variable(context, scope, name->buffer, flags, type_name, expr);
}

static bool tea_check_field_mutability(
  const tea_scope_t* scope, const tea_ast_node_t* field_access_node)
{
  tea_ast_node_t* object_node = field_access_node->field_access.object;
  if (!object_node || !object_node->token) {
    rtl_log_err("Internal error: Field access expression missing object component in AST");
    return false;
  }

  const tea_token_t* object_name = object_node->token;
  const tea_variable_t* variable = tea_scope_find_variable(scope, object_name->buffer);
  if (!variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' not found in current scope when checking field mutability, "
      "line: %d, column: %d",
      object_name->buffer, object_name->line, object_name->column);
    return false;
  }

  if (!(variable->flags & TEA_VARIABLE_MUTABLE)) {
    rtl_log_err(
      "Runtime error: Cannot modify field of immutable variable '%s' at line %d, column %d",
      object_name->buffer, object_name->line, object_name->column);
    return false;
  }

  return true;
}

static bool tea_perform_assignment(tea_value_t* target_value, const tea_value_t new_value,
  const bool is_optional, const char* target_name, const tea_token_t* error_token)
{
  const bool new_is_null = new_value.type == TEA_VALUE_NULL;
  const bool types_match = new_value.type == target_value->type;
  const bool null_type_match =
    target_value->type == TEA_VALUE_NULL && target_value->null_type == new_value.type;

  if (is_optional && new_is_null) {
    if (new_value.null_type == TEA_VALUE_NULL) {
      target_value->null_type = target_value->type;
      target_value->type = TEA_VALUE_NULL;
    } else {
      *target_value = new_value;
    }
  } else if (types_match || null_type_match) {
    *target_value = new_value;
  } else {
    rtl_log_err(
      "Runtime error: Type mismatch in assignment to '%s%s' at line %d, column %d: cannot "
      "assign %s value to %s target",
      target_name, is_optional ? "?" : "", error_token->line, error_token->column,
      tea_value_get_type_string(new_value.type), tea_value_get_type_string(target_value->type));
    return false;
  }

  switch (target_value->type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("New value for %s : %s = %d", target_name,
        tea_value_get_type_string(target_value->type), target_value->i32);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("New value for %s : %s = %f", target_name,
        tea_value_get_type_string(target_value->type), target_value->f32);
      break;
    default:
      break;
  }

  return true;
}

bool tea_interpret_assign(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_ast_node_t* lhs = node->binop.lhs;
  const tea_ast_node_t* rhs = node->binop.rhs;

  const tea_value_t new_value = tea_interpret_evaluate_expression(context, scope, rhs);
  if (new_value.type == TEA_VALUE_INVALID) {
    rtl_log_err("Runtime error: Failed to evaluate right-hand side expression in assignment");
    return false;
  }

  if (lhs->type != TEA_AST_NODE_IDENT) {
    if (!tea_check_field_mutability(scope, lhs)) {
      return false;
    }

    tea_value_t* field_value = tea_get_field_pointer(context, scope, lhs);
    if (!field_value) {
      return false;
    }

    const tea_ast_node_t* field_node = lhs->field_access.field;
    const tea_token_t* field_token = field_node ? field_node->token : NULL;
    const char* field_name = field_token ? field_token->buffer : "field";

    return tea_perform_assignment(field_value, new_value, false, field_name, field_token);
  }

  const tea_token_t* name = lhs->token;
  tea_variable_t* variable = tea_scope_find_variable(scope, name->buffer);
  if (!variable) {
    rtl_log_err("Runtime error: Undefined variable '%s' used in assignment at line %d, column %d",
      name->buffer, name->line, name->column);
    return false;
  }

  if (!(variable->flags & TEA_VARIABLE_MUTABLE)) {
    rtl_log_err("Runtime error: Cannot modify immutable variable '%s' at line %d, column %d",
      name->buffer, name->line, name->column);
    return false;
  }

  const bool is_optional = variable->flags & TEA_VARIABLE_OPTIONAL;
  return tea_perform_assignment(&variable->value, new_value, is_optional, name->buffer, name);
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
  if (cond_val.type == TEA_VALUE_INVALID) {
    tea_scope_cleanup(context, &inner_scope);
    return false;
  }

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

  while (true) {
    const tea_value_t cond_val = tea_interpret_evaluate_expression(context, scope, while_cond);
    if (cond_val.type == TEA_VALUE_INVALID) {
      return false;
    }
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
      if (return_context->returned_value.type == TEA_VALUE_INVALID) {
        return false;
      }
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

  rtl_log_err("Runtime error: 'break' statement can only be used inside loops");
  return false;
}

bool tea_interpret_execute_continue(tea_loop_context_t* loop_context)
{
  if (loop_context) {
    loop_context->is_continue_set = true;
    return true;
  }

  rtl_log_err("Runtime error: 'continue' statement can only be used inside loops");
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

  if (!node) {
    rtl_log_err("Node is null");
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
    case TEA_AST_NODE_TRAIT:
      return tea_interpret_trait_declaration(context, node);
    case TEA_AST_NODE_TRAIT_IMPL:
      return tea_interpret_trait_implementation(context, node);
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
        rtl_log_err(
          "Interpreter error: Unimplemented statement type <%s> in file %s, token: <%s> '%.*s' "
          "(line %d, col %d)",
          tea_ast_node_get_type_name(node->type), context->filename,
          tea_token_get_name(token->type), token->buffer_size, token->buffer, token->line,
          token->column);
      } else {
        rtl_log_err("Interpreter error: Unimplemented statement type <%s> in file %s",
          tea_ast_node_get_type_name(node->type), context->filename);
      }
    } break;
  }

  return false;
}
