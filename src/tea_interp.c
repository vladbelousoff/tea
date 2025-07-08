#include "tea_interp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rtl_log.h"
#include "rtl_memory.h"
#include "tea.h"
#include "tea_token.h"

static tea_value_t tea_value_create_float(const float value)
{
  tea_value_t result = { 0 };
  result.type = TEA_VALUE_FLOAT;
  result.float_value = value;
  return result;
}

static tea_value_t tea_value_create_undefined()
{
  tea_value_t result = { 0 };
  result.type = TEA_VALUE_UNDEFINED;
  return result;
}

static tea_value_t tea_value_binop(
  const tea_value_t left, const tea_value_t right, const tea_token_t *op)
{
  if (left.type == TEA_VALUE_UNDEFINED || right.type == TEA_VALUE_UNDEFINED) {
    return tea_value_create_undefined();
  }

  // Both operands should be floats
  if (left.type == TEA_VALUE_FLOAT && right.type == TEA_VALUE_FLOAT) {
    switch (op->type) {
      case TEA_TOKEN_PLUS:
        return tea_value_create_float(left.float_value + right.float_value);
      case TEA_TOKEN_MINUS:
        return tea_value_create_float(left.float_value - right.float_value);
      case TEA_TOKEN_STAR:
        return tea_value_create_float(left.float_value * right.float_value);
      case TEA_TOKEN_SLASH:
        if (right.float_value == 0.0f) {
          rtl_log_err("Division by zero!");
          break;
        }
        return tea_value_create_float(left.float_value / right.float_value);
      case TEA_TOKEN_EQ:
        return tea_value_create_float(left.float_value == right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_NE:
        return tea_value_create_float(left.float_value != right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_GE:
        return tea_value_create_float(left.float_value >= right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_LE:
        return tea_value_create_float(left.float_value <= right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_GT:
        return tea_value_create_float(left.float_value > right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_LT:
        return tea_value_create_float(left.float_value < right.float_value ? 1.0f : 0.0f);
      case TEA_TOKEN_AND:
        return tea_value_create_float(
          (left.float_value != 0.0f && right.float_value != 0.0f) ? 1.0f : 0.0f);
      case TEA_TOKEN_OR:
        return tea_value_create_float(
          (left.float_value != 0.0f || right.float_value != 0.0f) ? 1.0f : 0.0f);
      default:
        break;
    }
  }

  return tea_value_create_undefined();
}

static tea_value_t tea_value_unop(const tea_value_t operand, const char *op)
{
  if (operand.type == TEA_VALUE_UNDEFINED) {
    return tea_value_create_undefined();
  }

  if (strcmp(op, "-") == 0 && operand.type == TEA_VALUE_FLOAT) {
    return tea_value_create_float(-operand.float_value);
  }

  return tea_value_create_undefined();
}

// Variable management functions
static tea_variable_t *tea_context_find_variable(const tea_context_t *context, const char *name)
{
  tea_variable_t *var = context->variables;
  while (var) {
    if (strcmp(var->name, name) == 0) {
      return var;
    }
    var = var->next;
  }
  return NULL;
}

static bool tea_context_set_variable(
  tea_context_t *context, const char *name, const tea_value_t value, const bool is_mutable)
{
  tea_variable_t *existing = tea_context_find_variable(context, name);

  if (existing) {
    if (!existing->is_mutable) {
      rtl_log_err("Cannot assign to immutable variable '%s'", name);
      return false;
    }
    existing->value = value;
    return true;
  }

  tea_variable_t *var = rtl_malloc(sizeof(tea_variable_t));
  var->name = rtl_malloc(strlen(name) + 1);
  strcpy(var->name, name);
  var->value = value;
  var->is_mutable = is_mutable;
  var->next = context->variables;
  context->variables = var;

  return true;
}

static tea_value_t tea_context_get_variable(const tea_context_t *context, const char *name)
{
  const tea_variable_t *var = tea_context_find_variable(context, name);
  if (var) {
    return var->value;
  }

  rtl_log_err("Undefined variable '%s'", name);
  return tea_value_create_undefined();
}

void tea_interp_init(tea_context_t *context)
{
  context->variables = NULL;
}

void tea_interp_cleanup(tea_context_t *context)
{
  tea_variable_t *var = context->variables;
  while (var) {
    tea_variable_t *next = var->next;
    rtl_free(var->name);
    rtl_free(var);
    var = next;
  }
  context->variables = NULL;
}

void tea_value_print(const char *name, const tea_value_t *value)
{
  switch (value->type) {
    case TEA_VALUE_FLOAT:
      rtl_log_dbg("%s = %.2f", name, value->float_value);
      break;
    case TEA_VALUE_UNDEFINED:
      rtl_log_err("%s is undefined", name);
      break;
  }
}

static tea_value_t tea_interp_evaluate_expression(tea_context_t *context, tea_ast_node_t *node);

tea_value_t tea_interp_evaluate(tea_context_t *context, tea_ast_node_t *node)
{
  return tea_interp_evaluate_expression(context, node);
}

static tea_value_t tea_interp_evaluate_expression(tea_context_t *context, tea_ast_node_t *node)
{
  if (!node) {
    return tea_value_create_undefined();
  }

  switch (node->type) {
    case TEA_AST_NODE_NUMBER: {
      if (node->token && node->token->buffer_size > 0) {
        char *buffer = rtl_malloc(node->token->buffer_size + 1);
        memcpy(buffer, node->token->buffer, node->token->buffer_size);
        buffer[node->token->buffer_size] = '\0';

        // Always parse as float
        const float value = strtof(buffer, NULL);
        rtl_free(buffer);
        return tea_value_create_float(value);
      }
      return tea_value_create_undefined();
    }

    case TEA_AST_NODE_IDENT: {
      if (node->token && node->token->buffer_size > 0) {
        char *name = rtl_malloc(node->token->buffer_size + 1);
        memcpy(name, node->token->buffer, node->token->buffer_size);
        name[node->token->buffer_size] = '\0';

        const tea_value_t value = tea_context_get_variable(context, name);
        rtl_free(name);
        return value;
      }
      return tea_value_create_undefined();
    }

    case TEA_AST_NODE_BINOP: {
      if (!node->binop.lhs || !node->binop.rhs) {
        return tea_value_create_undefined();
      }

      const tea_value_t lhs_val = tea_interp_evaluate_expression(context, node->binop.lhs);
      const tea_value_t rhs_val = tea_interp_evaluate_expression(context, node->binop.rhs);

      const tea_token_t *token = node->token;
      if (token) {
        return tea_value_binop(lhs_val, rhs_val, token);
      }

      return tea_value_create_undefined();
    }

    case TEA_AST_NODE_UNARY: {
      rtl_list_entry_t *entry = rtl_list_first(&node->children);
      if (!entry) {
        return tea_value_create_undefined();
      }

      tea_ast_node_t *operand = rtl_list_record(entry, tea_ast_node_t, link);
      const tea_value_t operand_val = tea_interp_evaluate_expression(context, operand);

      // Use the operator token stored in the AST node
      if (node->token && node->token->buffer_size > 0) {
        char *op = rtl_malloc(node->token->buffer_size + 1);
        memcpy(op, node->token->buffer, node->token->buffer_size);
        op[node->token->buffer_size] = '\0';

        const tea_value_t result = tea_value_unop(operand_val, op);
        rtl_free(op);
        return result;
      }

      return tea_value_create_undefined();
    }

    default:
      return tea_value_create_undefined();
  }
}

bool tea_interp_execute(tea_context_t *context, tea_ast_node_t *node)
{
  if (!node) {
    return true;
  }

  switch (node->type) {
    case TEA_AST_NODE_PROGRAM: {
      rtl_list_entry_t *entry;
      rtl_list_for_each(entry, &node->children)
      {
        tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
        if (!tea_interp_execute(context, child)) {
          return false;
        }
      }
      return true;
    }
    case TEA_AST_NODE_LET:
    case TEA_AST_NODE_LET_MUT: {
      if (!node->token || node->token->buffer_size == 0) {
        rtl_log_err("Let statement missing variable name");
        return false;
      }

      char *name = rtl_malloc(node->token->buffer_size + 1);
      memcpy(name, node->token->buffer, node->token->buffer_size);
      name[node->token->buffer_size] = '\0';

      rtl_list_entry_t *entry = rtl_list_first(&node->children);
      if (!entry) {
        rtl_log_err("Let statement missing expression");
        rtl_free(name);
        return false;
      }

      tea_ast_node_t *expr = rtl_list_record(entry, tea_ast_node_t, link);
      const tea_value_t value = tea_interp_evaluate_expression(context, expr);

      const bool is_mutable = node->type == TEA_AST_NODE_LET_MUT;
      const bool success = tea_context_set_variable(context, name, value, is_mutable);

      if (success) {
        tea_value_print(name, &value);
      }

      rtl_free(name);
      return success;
    }
    case TEA_AST_NODE_ASSIGN: {
      if (!node->token || node->token->buffer_size == 0) {
        rtl_log_err("Assignment statement missing variable name");
        return false;
      }

      char *name = rtl_malloc(node->token->buffer_size + 1);
      memcpy(name, node->token->buffer, node->token->buffer_size);
      name[node->token->buffer_size] = '\0';

      rtl_list_entry_t *entry = rtl_list_first(&node->children);
      if (!entry) {
        rtl_log_err("Assignment statement missing expression");
        rtl_free(name);
        return false;
      }

      tea_ast_node_t *expr = rtl_list_record(entry, tea_ast_node_t, link);
      const tea_value_t value = tea_interp_evaluate_expression(context, expr);

      const bool success = tea_context_set_variable(context, name, value, true);

      if (success) {
        tea_value_print(name, &value);
      }

      rtl_free(name);
      return success;
    }
    case TEA_AST_NODE_FUNCTION:
    case TEA_AST_NODE_FUNCTION_MUT:
      // Skip functions for now
      return true;
    default:
      // For other node types, try to execute children
      rtl_list_entry_t *entry;
      rtl_list_for_each(entry, &node->children)
      {
        tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
        if (!tea_interp_execute(context, child)) {
          return false;
        }
      }
      return true;
  }
}
