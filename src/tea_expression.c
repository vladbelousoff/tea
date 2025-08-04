#include "tea_expression.h"

#include "tea_function.h"
#include "tea_struct.h"

#include <stdlib.h>

#include <rtl.h>
#include <rtl_log.h>

#include "tea_grammar.h"

tea_value_t tea_interpret_evaluate_integer_number(tea_token_t* token)
{
  tea_value_t value;
  value.type = TEA_VALUE_I32;
  value.i32 = *(int*)&token->buffer;

  return value;
}

tea_value_t tea_interpret_evaluate_float_number(tea_token_t* token)
{
  tea_value_t value;
  value.type = TEA_VALUE_F32;
  value.f32 = *(float*)&token->buffer;

  return value;
}

tea_value_t tea_interpret_evaluate_binop(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_value_t lhs_val = tea_interpret_evaluate_expression(context, scope, node->binop.lhs);
  const tea_value_t rhs_val = tea_interpret_evaluate_expression(context, scope, node->binop.rhs);

  return tea_value_binop(lhs_val, rhs_val, node->token);
}

tea_value_t tea_interpret_evaluate_unary(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  rtl_list_entry_t* entry = rtl_list_first(&node->children);
  const tea_ast_node_t* operand = rtl_list_record(entry, tea_ast_node_t, link);
  tea_value_t operand_val = tea_interpret_evaluate_expression(context, scope, operand);
  const tea_token_t* token = node->token;

  switch (token->type) {
    case TEA_TOKEN_PLUS:
      break;
    case TEA_TOKEN_MINUS:
      switch (operand_val.type) {
        case TEA_VALUE_I32:
          operand_val.i32 = -operand_val.i32;
          break;
        case TEA_VALUE_F32:
          operand_val.f32 = -operand_val.f32;
          break;
        default:
          break;
      }
      break;
    case TEA_TOKEN_EXCLAMATION_MARK:
      switch (operand_val.type) {
        case TEA_VALUE_I32:
          operand_val.i32 = !operand_val.i32;
          break;
        default:
          break;
      }
      break;
    default:
      rtl_log_err("Expression evaluation error: Invalid unary operator '%s' at line %d, column %d",
        token->buffer, token->line, token->column);
      break;
  }

  // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
  return operand_val;
}

tea_value_t tea_interpret_evaluate_ident(const tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Internal error: Missing token for identifier node during expression evaluation");
    return tea_value_invalid();
  }

  const tea_variable_t* variable = tea_scope_find_variable(scope, token->buffer);
  if (!variable) {
    rtl_log_err("Runtime error: Undefined variable '%s' referenced at line %d, column %d",
      token->buffer, token->line, token->column);
    return tea_value_invalid();
  }

  return variable->value;
}

tea_value_t tea_interpret_evaluate_string(const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err(
      "Internal error: Missing token for string literal node during expression evaluation");
    return tea_value_invalid();
  }

  tea_instance_t* object = rtl_malloc(sizeof(tea_instance_t) + token->buffer_size + 1);
  if (!object) {
    rtl_log_err(
      "Memory error: Failed to allocate memory for struct '%s' instance at line %d, column %d",
      token->buffer, token->line, token->column);
    return tea_value_invalid();
  }

  object->type = "string";
  memcpy(object->buffer, token->buffer, token->buffer_size + 1);

  const tea_value_t result = { .type = TEA_VALUE_INSTANCE, .object = object };
  return result;
}

tea_value_t tea_interpret_evaluate_expression(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  if (!node) {
    rtl_log_err("Internal error: Null AST node passed to expression evaluator");
    return tea_value_invalid();
  }

  switch (node->type) {
    case TEA_AST_NODE_INTEGER_NUMBER:
      return tea_interpret_evaluate_integer_number(node->token);
    case TEA_AST_NODE_FLOAT_NUMBER:
      return tea_interpret_evaluate_float_number(node->token);
    case TEA_AST_NODE_BINOP:
      return tea_interpret_evaluate_binop(context, scope, node);
    case TEA_AST_NODE_UNARY:
      return tea_interpret_evaluate_unary(context, scope, node);
    case TEA_AST_NODE_IDENT:
      return tea_interpret_evaluate_ident(scope, node);
    case TEA_AST_NODE_STRING:
      return tea_interpret_evaluate_string(node);
    case TEA_AST_NODE_FUNCTION_CALL:
      return tea_interpret_evaluate_function_call(context, scope, node);
    case TEA_AST_NODE_STRUCT_INSTANCE:
      return tea_interpret_evaluate_new(context, scope, node);
    case TEA_AST_NODE_FIELD_ACCESS:
      return tea_interpret_field_access(context, scope, node);
    case TEA_AST_NODE_NULL:
      return tea_value_null();
    default: {
      tea_token_t* token = node->token;
      if (token) {
        rtl_log_err(
          "Expression evaluation error: Unsupported node type <%s> with token <%s> '%.*s' at line "
          "%d, column %d",
          tea_ast_node_get_type_name(node->type), tea_token_get_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      } else {
        rtl_log_err("Expression evaluation error: Unsupported node type <%s> (no token available)",
          tea_ast_node_get_type_name(node->type));
      }
    } break;
  }

  return tea_value_invalid();
}
