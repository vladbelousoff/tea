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

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent_scope)
{
  scope->parent_scope = parent_scope;
  rtl_list_init(&scope->variables);
}

void tea_scope_cleanup(const tea_scope_t* scope)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &scope->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    if (variable->value.type == TEA_VALUE_STRING) {
      rtl_free(variable->value.string_value);
    }
    rtl_free(variable);
  }
}

void tea_interpret_init(tea_context_t* context)
{
  rtl_list_init(&context->functions);
}

void tea_interpret_cleanup(const tea_context_t* context)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->functions)
  {
    tea_function_t* function = rtl_list_record(entry, tea_function_t, link);
    rtl_list_remove(entry);
    if (function->params && function->param_count > 0) {
      rtl_free(function->params);
    }
    rtl_free(function);
  }
}

static bool tea_interpret_execute_stmt(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    if (!tea_interpret_execute(context, scope, child)) {
      return false;
    }
  }

  return true;
}

static tea_variable_t* tea_context_find_variable(const tea_scope_t* scope, const char* name)
{
  const tea_scope_t* current_scope = scope;
  while (current_scope) {
    rtl_list_entry_t* entry;
    rtl_list_for_each(entry, &current_scope->variables)
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

    current_scope = current_scope->parent_scope;
  }

  return NULL;
}

static bool tea_declare_variable(tea_context_t* context, tea_scope_t* scope,
  const tea_token_t* name, const bool is_mutable, const tea_ast_node_t* type,
  const tea_ast_node_t* initial_value)
{
  if (!name) {
    return false;
  }

  tea_variable_t* variable = tea_context_find_variable(scope, name->buffer);
  if (variable) {
    rtl_log_err("Variable %s is already declared (line: %d, col: %d)", name->buffer, name->line,
      name->column);
    return false;
  }

  variable = rtl_malloc(sizeof(*variable));
  if (!variable) {
    rtl_log_err("Failed to allocate memory for variable %.*s", name->buffer_size, name->buffer);
    return false;
  }

  variable->name = name;
  variable->is_mutable = is_mutable;
  variable->value = tea_interpret_evaluate_expression(context, scope, initial_value);

  switch (variable->value.type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("Declare variable %s : %s = %d", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.i32_value);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("Declare variable %s : %s = %f", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.f32_value);
      break;
    case TEA_VALUE_STRING:
      rtl_log_dbg("Declare variable %s : %s = '%s'", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.string_value);
    default:
      break;
  }

  rtl_list_add_tail(&scope->variables, &variable->link);

  return true;
}

static bool tea_interpret_execute_let(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
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

  return tea_declare_variable(context, scope, name, is_mutable, type, expr);
}

static bool tea_interpret_execute_assign(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* name = node->token;
  if (!name) {
    rtl_log_err(
      "Can't process node %s, because it's name is null", tea_ast_node_get_type_name(node->type));
    exit(1);
  }

  tea_variable_t* variable = tea_context_find_variable(scope, name->buffer);
  if (!variable) {
    rtl_log_err("Cannot find variable '%s'", name->buffer);
    exit(1);
  }

  if (!variable->is_mutable) {
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
        tea_value_get_type_string(variable->value.type), variable->value.i32_value);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("New value for variable %s : %s = %f", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.f32_value);
      break;
    case TEA_VALUE_STRING:
      rtl_log_dbg("New value for variable %s : %s = '%s'", name->buffer,
        tea_value_get_type_string(variable->value.type), variable->value.string_value);
      break;
    case TEA_VALUE_OBJECT:
      break;
  }

  return true;
}

static bool tea_interpret_execute_if(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
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

  bool result = false;
  if (cond_val.i32_value != 0) {
    result = tea_interpret_execute(context, &inner_scope, then_node);
  } else if (else_node) {
    result = tea_interpret_execute(context, &inner_scope, else_node);
  }

  tea_scope_cleanup(&inner_scope);
  return result;
}

static bool tea_interpret_execute_while(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
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

  while (true) {
    const tea_value_t cond_val = tea_interpret_evaluate_expression(context, scope, while_cond);
    if (cond_val.i32_value == 0) {
      break;
    }

    tea_scope_t inner_scope;
    tea_scope_init(&inner_scope, scope);
    const bool result = tea_interpret_execute(context, &inner_scope, while_body);
    tea_scope_cleanup(&inner_scope);

    if (!result) {
      break;
    }
  }

  return true;
}

static tea_function_param_t* tea_construct_function_params(
  const tea_ast_node_t* fn_params, int* param_count)
{
  int index = 0;
  rtl_list_entry_t* entry;

  if (fn_params) {
    rtl_list_for_each(entry, &fn_params->children)
    {
      index++;
    }
  }

  if (index == 0) {
    *param_count = 0;
    return NULL;
  }

  tea_function_param_t* params = rtl_malloc(sizeof(*params) * index);
  if (!params) {
    return NULL;
  }

  index = 0;
  rtl_list_for_each(entry, &fn_params->children)
  {
    const tea_ast_node_t* param_name_node = rtl_list_record(entry, tea_ast_node_t, link);
    if (!param_name_node) {
      continue;
    }

    const tea_ast_node_t* param_type_node =
      rtl_list_record(rtl_list_first(&param_name_node->children), tea_ast_node_t, link);
    if (!param_type_node) {
      continue;
    }

    const tea_token_t* param_name = param_name_node->token;
    if (!param_name) {
      continue;
    }

    const tea_token_t* param_type = param_type_node->token;
    if (!param_type) {
      continue;
    }

    params[index].name = param_name;
    params[index].type = param_type;

    rtl_log_dbg("Register param '%s' with type '%s'", param_name->buffer, param_type->buffer);

    index++;
  }

  *param_count = index;
  return params;
}

static bool tea_interpret_execute_function_declaration(
  tea_context_t* context, const tea_ast_node_t* node)
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
  fn->is_mutable = is_mutable;
  fn->params = tea_construct_function_params(fn_params, &fn->param_count);
  if (fn_return_type) {
    fn->return_type = fn_return_type->token;
  } else {
    fn->return_type = NULL;
  }

  rtl_list_add_tail(&context->functions, &fn->link);

  if (fn->return_type) {
    rtl_log_dbg("Declare function: '%.*s' -> %.*s", fn_name->buffer_size, fn_name->buffer,
      fn->return_type->buffer_size, fn->return_type->buffer);
  } else {
    rtl_log_dbg("Declare function: '%.*s'", fn_name->buffer_size, fn_name->buffer);
  }

  return true;
}

static bool tea_interpret_execute_return(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  rtl_list_entry_t* entry = rtl_list_first(&node->children);
  if (entry) {
    const tea_ast_node_t* expr = rtl_list_record(entry, tea_ast_node_t, link);
    if (expr) {
      context->returned_value = tea_interpret_evaluate_expression(context, scope, expr);
    }
  }

  return true;
}

bool tea_interpret_execute(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  switch (node->type) {
    case TEA_AST_NODE_LET:
      return tea_interpret_execute_let(context, scope, node);
    case TEA_AST_NODE_ASSIGN:
      return tea_interpret_execute_assign(context, scope, node);
    case TEA_AST_NODE_IF:
      return tea_interpret_execute_if(context, scope, node);
    case TEA_AST_NODE_WHILE:
      return tea_interpret_execute_while(context, scope, node);
    case TEA_AST_NODE_FUNCTION:
      return tea_interpret_execute_function_declaration(context, node);
    case TEA_AST_NODE_RETURN:
      return tea_interpret_execute_return(context, scope, node);
    case TEA_AST_NODE_PROGRAM:
    case TEA_AST_NODE_STMT:
    case TEA_AST_NODE_THEN:
    case TEA_AST_NODE_ELSE:
    case TEA_AST_NODE_WHILE_COND:
    case TEA_AST_NODE_WHILE_BODY:
      return tea_interpret_execute_stmt(context, scope, node);
    default: {
      const tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Not implemented node: %s, token: <%s> '%.*s' (line %d, col %d)",
          tea_ast_node_get_type_name(node->type), tea_token_get_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      } else {
        rtl_log_err("Not implemented node: %s", tea_ast_node_get_type_name(node->type));
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
      case TEA_TOKEN_EQ:                                                                           \
        result = a == b;                                                                           \
        break;                                                                                     \
      case TEA_TOKEN_NE:                                                                           \
        result = a != b;                                                                           \
        break;                                                                                     \
      case TEA_TOKEN_GT:                                                                           \
        result = a > b;                                                                            \
        break;                                                                                     \
      case TEA_TOKEN_GE:                                                                           \
        result = a >= b;                                                                           \
        break;                                                                                     \
      case TEA_TOKEN_LT:                                                                           \
        result = a < b;                                                                            \
        break;                                                                                     \
      case TEA_TOKEN_LE:                                                                           \
        result = a <= b;                                                                           \
        break;                                                                                     \
    }                                                                                              \
  } while (0)

static tea_value_t tea_value_binop(
  const tea_value_t lhs_val, const tea_value_t rhs_val, const tea_token_t* op)
{
  tea_value_t result;

  // By-default it's i32 for boolean ops
  result.type = TEA_VALUE_I32;

  switch (op->type) {
    case TEA_TOKEN_OR:
      result.i32_value = lhs_val.i32_value || rhs_val.i32_value;
      return result;
    case TEA_TOKEN_AND:
      result.i32_value = lhs_val.i32_value && rhs_val.i32_value;
      return result;
    default:
      break;
  }

  if (lhs_val.type == TEA_VALUE_I32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      result.type = TEA_VALUE_I32;
      TEA_APPLY_BINOP(lhs_val.i32_value, rhs_val.i32_value, op->type, result.i32_value);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.i32_value, rhs_val.f32_value, op->type, result.f32_value);
      return result;
    }
  }

  if (lhs_val.type == TEA_VALUE_F32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.f32_value, rhs_val.i32_value, op->type, result.f32_value);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
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

static tea_value_t tea_interpret_evaluate_binop(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_value_t lhs_val = tea_interpret_evaluate_expression(context, scope, node->binop.lhs);
  const tea_value_t rhs_val = tea_interpret_evaluate_expression(context, scope, node->binop.rhs);

  return tea_value_binop(lhs_val, rhs_val, node->token);
}

static tea_value_t tea_interpret_evaluate_unary(
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

static tea_value_t tea_interpret_evaluate_ident(
  const tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Impossible to evaluate ident token");
    exit(1);
  }

  const tea_variable_t* variable = tea_context_find_variable(scope, token->buffer);
  if (!variable) {
    rtl_log_err("Can't find variable %s", token->buffer);
    exit(1);
  }

  return variable->value;
}

static tea_value_t tea_interpret_evaluate_string(const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Impossible to evaluate string token");
    exit(1);
  }

  tea_value_t result;
  result.type = TEA_VALUE_STRING;
  result.string_value = rtl_strdup(token->buffer);

  return result;
}

static const tea_function_t* tea_context_find_function(
  const tea_context_t* context, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->functions)
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

static tea_value_t tea_interpret_evaluate_function_call(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* token = node->token;
  if (!token) {
    rtl_log_err("Impossible to evaluate ident token");
    exit(1);
  }

  const tea_function_t* function = tea_context_find_function(context, token->buffer);
  if (!function) {
    rtl_log_err("Can't find function %s", token->buffer);
    exit(1);
  }

  tea_scope_t inner_scope;
  tea_scope_init(&inner_scope, scope);

  tea_value_t result = { 0 };
  if (function->param_count > 0) {
    rtl_log_err("Functions with params are not supported yet, just returning 0");
  } else {
    if (tea_interpret_execute(context, &inner_scope, function->body)) {
      result = context->returned_value;
    }
  }

  tea_scope_cleanup(&inner_scope);
  return result;
}

tea_value_t tea_interpret_evaluate_expression(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  if (!node) {
    rtl_log_err("Node %s is null!", tea_ast_node_get_type_name(node->type));
    exit(1);
  }

  switch (node->type) {
    case TEA_AST_NODE_NUMBER:
      return tea_interpret_evaluate_number(node->token);
    case TEA_AST_NODE_BINOP:
      return tea_interpret_evaluate_binop(context, scope, node);
    case TEA_AST_NODE_UNARY:
      return tea_interpret_evaluate_unary(context, scope, node);
    case TEA_AST_NODE_IDENT:
      return tea_interpret_evaluate_ident(scope, node);
    case TEA_AST_NODE_STRING:
      return tea_interpret_evaluate_string(node);
    case TEA_AST_NODE_CALL:
      return tea_interpret_evaluate_function_call(context, scope, node);
    default: {
      rtl_log_err("Failed to evaluate node <%s>", tea_ast_node_get_type_name(node->type));
      tea_token_t* token = node->token;
      if (token) {
        rtl_log_err("Token: <%s> %.*s (line: %d, column: %d)", tea_token_get_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      }
    } break;
  }

  exit(1);
}
