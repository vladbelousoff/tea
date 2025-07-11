#include "tea_interpret.h"

#include <stdlib.h>

#include "rtl_log.h"
#include "rtl_memory.h"
#include "tea.h"

const char* tea_value_get_type_string(const tea_value_type_t type)
{
  switch (type) {
    case TEA_VALUE_I32:
      return "i32";
    case TEA_VALUE_F32:
      return "f32";
    case TEA_VALUE_STRING:
      return "string";
    case TEA_VALUE_OBJECT:
      return "object";
  }

  return "UNKNOWN";
}

void tea_interpret_init(tea_context_t* context)
{
  rtl_list_init(&context->variables);
}

void tea_interpret_cleanup(const tea_context_t* context)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;
  rtl_list_for_each_safe(entry, safe, &context->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    if (variable->value.type == TEA_VALUE_STRING) {
      free((void*)variable->value.string_value);
    }
    rtl_free(variable);
  }
}

static bool tea_interpret_execute_program(tea_context_t* context, const tea_ast_node_t* node)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    if (!tea_interpret_execute(context, child)) {
      return false;
    }
  }

  return true;
}

static bool declare_variable(tea_context_t* context, const tea_token_t* name, const bool is_mutable,
  const tea_ast_node_t* type, const tea_ast_node_t* initial_value)
{
  if (!name) {
    return false;
  }

  tea_variable_t* variable = rtl_malloc(sizeof(*variable));
  if (!variable) {
    rtl_log_err("Failed to allocate memory for variable %.*s", name->buffer_size, name->buffer);
    return false;
  }

  variable->name = name;
  variable->is_mutable = is_mutable;
  variable->value = tea_interpret_evaluate_expression(context, initial_value);

  switch (variable->value.type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("Declare variable '%s' : %s = %d", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.i32_value);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("Declare variable '%s' : %s = %f", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.f32_value);
      break;
    default:
      break;
  }

  rtl_list_add_tail(&context->variables, &variable->link);
  return true;
}

static bool tea_interpret_execute_let(tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_token_t* name = node->token;

  bool is_mutable = false;
  const tea_ast_node_t* type = NULL;
  const tea_ast_node_t* expr = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    switch (child->type) {
      case TEA_AST_NODE_MUT:
        is_mutable = true;
        break;
      case TEA_AST_NODE_TYPE_ANNOT:
        type = child;
        break;
      default:
        expr = child;
        break;
    }
  }

  return declare_variable(context, name, is_mutable, type, expr);
}

bool tea_interpret_execute(tea_context_t* context, const tea_ast_node_t* node)
{
  switch (node->type) {
    case TEA_AST_NODE_PROGRAM:
      return tea_interpret_execute_program(context, node);
    case TEA_AST_NODE_LET:
      return tea_interpret_execute_let(context, node);
    default: {
      rtl_log_err("Not implemented: %s", tea_ast_node_get_type_name(node->type));
      const tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Token: <%s> %.*s (line: %d, column: %d)", tea_get_token_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      }
    } break;
  }

  return false;
}

static tea_value_t tea_interpret_evaluate_number(tea_token_t* token)
{
  tea_value_t value;
  char* end;

  // Try i32 first
  const int i32_value = (int)strtol(token->buffer, &end, 10);
  if (*end == 0) {
    value.type = TEA_VALUE_I32;
    value.i32_value = i32_value;
    return value;
  }

  // Next try f32
  const float f32_value = strtof(token->buffer, &end);
  if (*end == 0) {
    value.type = TEA_VALUE_F32;
    value.f32_value = f32_value;
    return value;
  }

  rtl_log_err("Error parsing %s as a number!", token->buffer);
  exit(1);
}

#define TEA_APPLY_BINOP(a, b, op, result)                                                          \
  do {                                                                                             \
    switch (op) {                                                                                  \
      case TEA_TOKEN_PLUS:                                                                         \
        result = a + b;                                                                            \
        break;                                                                                     \
      case TEA_TOKEN_MINUS:                                                                        \
        result = a - b;                                                                            \
        break;                                                                                     \
      case TEA_TOKEN_STAR:                                                                         \
        result = a * b;                                                                            \
        break;                                                                                     \
      case TEA_TOKEN_SLASH:                                                                        \
        if (b == 0) {                                                                              \
          rtl_log_err("Can't divide by zero!");                                                    \
          exit(1);                                                                                 \
        }                                                                                          \
        result = a / b;                                                                            \
        break;                                                                                     \
    }                                                                                              \
  } while (0)

static tea_value_t tea_value_binop(
  const tea_value_t lhs_val, const tea_value_t rhs_val, const tea_token_t* op)
{
  if (lhs_val.type == TEA_VALUE_I32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      tea_value_t result = { 0 };
      result.type = TEA_VALUE_I32;
      TEA_APPLY_BINOP(lhs_val.i32_value, rhs_val.i32_value, op->type, result.i32_value);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
      tea_value_t result = { 0 };
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.i32_value, rhs_val.f32_value, op->type, result.f32_value);
      return result;
    }
  }

  if (lhs_val.type == TEA_VALUE_F32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      tea_value_t result = { 0 };
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.f32_value, rhs_val.i32_value, op->type, result.f32_value);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
      tea_value_t result = { 0 };
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.f32_value, rhs_val.f32_value, op->type, result.f32_value);
      return result;
    }
  }

  rtl_log_err("Binary operations for type %s and type %s are not implemented!",
    tea_value_get_type_string(lhs_val.type), tea_value_get_type_string(rhs_val.type));
  exit(1);
}

#undef TEA_APPLY_BINOP

static tea_value_t tea_interpret_evaluate_binop(tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_value_t lhs_val = tea_interpret_evaluate_expression(context, node->binop.lhs);
  const tea_value_t rhs_val = tea_interpret_evaluate_expression(context, node->binop.rhs);

  return tea_value_binop(lhs_val, rhs_val, node->token);
}

static tea_value_t tea_interpret_evaluate_unary(tea_context_t* context, const tea_ast_node_t* node)
{
  rtl_list_entry_t* entry = rtl_list_first(&node->children);
  const tea_ast_node_t* operand = rtl_list_record(entry, tea_ast_node_t, link);
  tea_value_t operand_val = tea_interpret_evaluate_expression(context, operand);
  const tea_token_t* token = node->token;

  switch (token->type) {
    case TEA_TOKEN_PLUS:
      break;
    case TEA_TOKEN_MINUS:
      switch (operand_val.type) {
        case TEA_VALUE_I32:
          operand_val.i32_value = -operand_val.i32_value;
          break;
        case TEA_VALUE_F32:
          operand_val.f32_value = -operand_val.f32_value;
          break;
        default:
          break;
      }
      break;
    default:
      rtl_log_err("Impossible to apply operator %s as unary", token->buffer);
      break;
  }

  // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
  return operand_val;
}

static tea_variable_t* tea_context_find_variable(const tea_context_t* context, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    const tea_token_t* variable_name = variable->name;
    if (!variable_name) {
      continue;
    }
    if (!strcmp(variable_name->buffer, name)) {
      return variable;
    }
  }

  return NULL;
}

static tea_value_t tea_interpret_evaluate_ident(
  const tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Impossible to evaluate ident token");
    exit(1);
  }

  const tea_variable_t* variable = tea_context_find_variable(context, token->buffer);
  if (!variable) {
    rtl_log_err("Can't find variable %s", token->buffer);
    exit(1);
  }

  return variable->value;
}

static tea_value_t tea_interpret_evaluate_string(
  const tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Impossible to evaluate string token");
    exit(1);
  }

  tea_value_t result;
  result.type = TEA_VALUE_STRING;
  result.string_value = _strdup(&token->buffer[0]);

  return result;
}

tea_value_t tea_interpret_evaluate_expression(tea_context_t* context, const tea_ast_node_t* node)
{
  if (!node) {
    rtl_log_err("Node %s is null!", tea_ast_node_get_type_name(node->type));
    exit(1);
  }

  switch (node->type) {
    case TEA_AST_NODE_NUMBER:
      return tea_interpret_evaluate_number(node->token);
    case TEA_AST_NODE_BINOP:
      return tea_interpret_evaluate_binop(context, node);
    case TEA_AST_NODE_UNARY:
      return tea_interpret_evaluate_unary(context, node);
    case TEA_AST_NODE_IDENT:
      return tea_interpret_evaluate_ident(context, node);
    case TEA_AST_NODE_STRING:
      return tea_interpret_evaluate_string(context, node);
    default: {
      rtl_log_err("Failed to evaluate node <%s>", tea_ast_node_get_type_name(node->type));
      tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Token: <%s> %.*s (line: %d, column: %d)", tea_get_token_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      }
    } break;
  }

  exit(1);
}
