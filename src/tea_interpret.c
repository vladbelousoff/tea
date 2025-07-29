#include "tea_interpret.h"

#include <assert.h>
#include <stdlib.h>

#include "rtl.h"
#include "rtl_log.h"
#include "rtl_memory.h"
#include "tea.h"

#define TEA_VARIABLE_POOL_ENABLED 1

const char* tea_value_get_type_string(const tea_value_type_t type)
{
  switch (type) {
    case TEA_VALUE_INVALID:
      return "unset";
    case TEA_VALUE_I32:
      return "i32";
    case TEA_VALUE_F32:
      return "f32";
    case TEA_VALUE_STRING:
      return "string";
    case TEA_VALUE_INSTANCE:
      return "object";
    case TEA_VALUE_NULL:
      return "null";
  }

  return "UNKNOWN";
}

typedef struct
{
  const char* name;
  tea_value_type_t type;
} tea_type_string_id;

tea_value_type_t tea_value_get_type_by_string(const char* name)
{
  static tea_type_string_id ids[] = { { "i32", TEA_VALUE_I32 }, { "f32", TEA_VALUE_F32 },
    { "string", TEA_VALUE_STRING }, { NULL, TEA_VALUE_INVALID } };

  for (int i = 0; ids[i].name; ++i) {
    if (!strcmp(name, ids[i].name)) {
      return ids[i].type;
    }
  }

  return TEA_VALUE_INVALID;
}

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

tea_value_t tea_value_invalid()
{
  static tea_value_t result = { .type = TEA_VALUE_INVALID };
  return result;
}

tea_value_t tea_value_null()
{
  // For the keyword 'null' we don't know the exact type, so let it be just null
  static tea_value_t result = { .type = TEA_VALUE_NULL, .null_type = TEA_VALUE_NULL };
  return result;
}

static tea_variable_t* tea_allocate_variable(const tea_context_t* context)
{
#if TEA_VARIABLE_POOL_ENABLED
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->variable_pool)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    return variable;
  }
#endif
  return rtl_malloc(sizeof(tea_variable_t));
}

void tea_free_variable(tea_context_t* context, tea_variable_t* variable)
{
#if TEA_VARIABLE_POOL_ENABLED
  if (variable) {
    rtl_list_add_head(&context->variable_pool, &variable->link);
  }
#else
  rtl_free(variable);
#endif
}

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent_scope)
{
  scope->parent_scope = parent_scope;
  rtl_list_init(&scope->variables);
}

void tea_scope_cleanup(tea_context_t* context, const tea_scope_t* scope)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &scope->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    tea_free_variable(context, variable);
  }
}

void tea_interpret_init(tea_context_t* context, const char* filename)
{
  context->filename = filename;
  rtl_list_init(&context->functions);
  rtl_list_init(&context->native_functions);
  rtl_list_init(&context->struct_declarations);
  rtl_list_init(&context->variable_pool);
}

void tea_interpret_cleanup(const tea_context_t* context)
{
  rtl_list_entry_t* entry;
  rtl_list_entry_t* safe;

  rtl_list_for_each_safe(entry, safe, &context->functions)
  {
    tea_function_t* function = rtl_list_record(entry, tea_function_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &context->native_functions)
  {
    tea_native_function_t* function = rtl_list_record(entry, tea_native_function_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &context->struct_declarations)
  {
    tea_struct_declaration_t* struct_declaration =
      rtl_list_record(entry, tea_struct_declaration_t, link);
    rtl_list_remove(entry);

    rtl_list_entry_t* function_entry;
    rtl_list_entry_t* function_entry_safe;
    rtl_list_for_each_safe(function_entry, function_entry_safe, &struct_declaration->functions)
    {
      tea_function_t* function = rtl_list_record(function_entry, tea_function_t, link);
      rtl_list_remove(function_entry);
      rtl_free(function);
    }

    rtl_free(struct_declaration);
  }

  rtl_list_for_each_safe(entry, safe, &context->variable_pool)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
    rtl_list_remove(entry);
    rtl_free(variable);
  }
}

static bool tea_interpret_execute_stmt(tea_context_t* context, tea_scope_t* scope,
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

static tea_variable_t* tea_context_find_variable_this_scope_only(
  const tea_scope_t* scope, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &scope->variables)
  {
    tea_variable_t* variable = rtl_list_record(entry, tea_variable_t, link);
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

static tea_variable_t* tea_context_find_variable(const tea_scope_t* scope, const char* name)
{
  const tea_scope_t* current_scope = scope;
  while (current_scope) {
    tea_variable_t* variable = tea_context_find_variable_this_scope_only(current_scope, name);
    if (variable) {
      return variable;
    }

    current_scope = current_scope->parent_scope;
  }

  return NULL;
}

static bool tea_declare_variable(tea_context_t* context, tea_scope_t* scope, const char* name,
  const unsigned int flags, const char* type, const tea_ast_node_t* initial_value)
{
  tea_variable_t* variable = tea_context_find_variable_this_scope_only(scope, name);
  if (variable) {
    rtl_log_err("Variable %s is already declared", variable->name);
    return false;
  }

  variable = tea_allocate_variable(context);
  if (!variable) {
    rtl_log_err("Failed to allocate memory for variable %s", name);
    return false;
  }

  variable->name = name;
  variable->flags = flags;
  tea_value_t value = tea_interpret_evaluate_expression(context, scope, initial_value);
  if (type) {
    const tea_value_type_t defined_type = tea_value_get_type_by_string(type);
    rtl_assert(defined_type != TEA_VALUE_INVALID, "Can't find type '%s'", type);
    if (value.type == TEA_VALUE_NULL) {
      value.type = defined_type;
    } else {
      rtl_assert(value.type == defined_type, "Type mismatch");
    }
  }
  variable->value = value;

  switch (variable->value.type) {
    case TEA_VALUE_I32:
      rtl_log_dbg("Declare variable %s : %s = %d", name,
        tea_value_get_type_string(variable->value.type), variable->value.i32);
      break;
    case TEA_VALUE_F32:
      rtl_log_dbg("Declare variable %s : %s = %f", name,
        tea_value_get_type_string(variable->value.type), variable->value.f32);
      break;
    case TEA_VALUE_STRING:
      rtl_log_dbg("Declare variable %s : %s = '%s'", name,
        tea_value_get_type_string(variable->value.type), variable->value.string);
    case TEA_VALUE_NULL:
      rtl_log_dbg(
        "Declare variable %s : %s = null", name, tea_value_get_type_string(variable->value.type));
    default:
      break;
  }

  rtl_list_add_tail(&scope->variables, &variable->link);

  return true;
}

static bool tea_interpret_let(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
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

static tea_value_t* tea_field_access(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node);

static bool tea_interpret_execute_assign(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
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

    tea_value_t* value = tea_field_access(context, scope, field_node);
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

  tea_variable_t* variable = tea_context_find_variable(scope, name->buffer);
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

static bool tea_interpret_execute_if(tea_context_t* context, tea_scope_t* scope,
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

static bool tea_interpret_execute_while(tea_context_t* context, tea_scope_t* scope,
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

static bool tea_declare_function(const tea_ast_node_t* node, rtl_list_entry_t* functions)
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

static bool tea_interpret_function_declaration(tea_context_t* context, const tea_ast_node_t* node)
{
  return tea_declare_function(node, &context->functions);
}

static bool tea_interpret_execute_return(tea_context_t* context, tea_scope_t* scope,
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

static tea_value_t tea_interpret_evaluate_function_call(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);

static bool tea_interpret_struct_declaration(tea_context_t* context, const tea_ast_node_t* node)
{
  tea_struct_declaration_t* struct_declaration = rtl_malloc(sizeof(*struct_declaration));
  if (!struct_declaration) {
    return false;
  }

  struct_declaration->node = node;
  struct_declaration->field_count = rtl_list_length(&node->children);
  rtl_list_init(&struct_declaration->functions);
  rtl_list_add_tail(&context->struct_declarations, &struct_declaration->link);

  tea_token_t* name = node->token;
  rtl_log_dbg("Declare struct '%s'", name ? name->buffer : "");

  return true;
}

static tea_struct_declaration_t* tea_find_struct_declaration(
  const tea_context_t* context, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->struct_declarations)
  {
    tea_struct_declaration_t* declaration = rtl_list_record(entry, tea_struct_declaration_t, link);
    assert(declaration);
    assert(declaration->node);
    const tea_token_t* declaration_token = declaration->node->token;
    if (declaration_token) {
      if (!strcmp(declaration_token->buffer, name)) {
        return declaration;
      }
    }
  }

  return NULL;
}

static bool tea_interpret_impl_block(const tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_token_t* block_name = node->token;
  rtl_assert(block_name, "Block name is not set!");

  tea_struct_declaration_t* struct_declaration =
    tea_find_struct_declaration(context, block_name->buffer);
  rtl_assert(
    struct_declaration, "You can't impl function for non declared struct %s!", block_name->buffer);

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    rtl_assert(child->type == TEA_AST_NODE_IMPL_ITEM, "Impl block must contain only impl items!");
    const rtl_list_entry_t* function_entry = rtl_list_first(&child->children);
    rtl_assert(function_entry, "Impl item contains no children!");
    const tea_ast_node_t* function_node = rtl_list_record(function_entry, tea_ast_node_t, link);
    rtl_assert(function_node && function_node->type == TEA_AST_NODE_FUNCTION,
      "There's no function in impl item!");

    const bool result = tea_declare_function(function_node, &struct_declaration->functions);
    if (!result) {
      return false;
    }
  }

  return true;
}

static bool tea_interpret_execute_break(tea_loop_context_t* loop_context)
{
  if (loop_context) {
    loop_context->is_break_set = true;
    return true;
  }

  rtl_log_err("Break cannot be used outside of loops");
  return false;
}

static bool tea_interpret_execute_continue(tea_loop_context_t* loop_context)
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
      return tea_interpret_execute_assign(context, scope, node);
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

static tea_value_t tea_interpret_evaluate_integer_number(tea_token_t* token)
{
  tea_value_t value;
  value.type = TEA_VALUE_I32;
  value.i32 = *(int*)&token->buffer;

  return value;
}

static tea_value_t tea_interpret_evaluate_float_number(tea_token_t* token)
{
  tea_value_t value;
  value.type = TEA_VALUE_F32;
  value.f32 = *(float*)&token->buffer;

  return value;
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
      result.i32 = lhs_val.i32 || rhs_val.i32;
      return result;
    case TEA_TOKEN_AND:
      result.i32 = lhs_val.i32 && rhs_val.i32;
      return result;
    default:
      break;
  }

  if (lhs_val.type == TEA_VALUE_I32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      result.type = TEA_VALUE_I32;
      TEA_APPLY_BINOP(lhs_val.i32, rhs_val.i32, op->type, result.i32);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.i32, rhs_val.f32, op->type, result.f32);
      return result;
    }
  }

  if (lhs_val.type == TEA_VALUE_F32) {
    if (rhs_val.type == TEA_VALUE_I32) {
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.f32, rhs_val.i32, op->type, result.f32);
      return result;
    }
    if (rhs_val.type == TEA_VALUE_F32) {
      result.type = TEA_VALUE_F32;
      TEA_APPLY_BINOP(lhs_val.f32, rhs_val.f32, op->type, result.f32);
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
          operand_val.i32 = -operand_val.i32;
          break;
        case TEA_VALUE_F32:
          operand_val.f32 = -operand_val.f32;
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
  result.string = token->buffer;

  return result;
}

static const tea_native_function_t* tea_context_find_native_function(
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

static const tea_function_t* tea_context_find_function(
  const rtl_list_entry_t* functions, const char* name)
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

static tea_value_t tea_interpret_evaluate_native_function_call(tea_context_t* context,
  tea_scope_t* scope, const tea_native_function_t* native_function,
  const tea_ast_node_t* function_call_args)
{
  tea_function_args_t function_args;
  rtl_list_init(&function_args.list_head);

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &function_call_args->children)
  {
    const tea_ast_node_t* arg_expr = rtl_list_record(entry, tea_ast_node_t, link);
    tea_variable_t* arg = tea_allocate_variable(context);
    rtl_assert(arg, "Failed to allocate variable!");
    arg->value = tea_interpret_evaluate_expression(context, scope, arg_expr);
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

static tea_value_t tea_interpret_evaluate_function_call(
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
    rtl_assert(field_node && object_node, "These fields can't be null");

    const tea_token_t* object_token = object_node->token;
    const tea_token_t* field_token = field_node->token;
    rtl_assert(field_token && object_token, "These fields can't be null");

    tea_variable_t* variable = tea_context_find_variable(scope, object_token->buffer);
    if (!variable) {
      rtl_log_err("Can't find variable %s", object_token->buffer);
      exit(1);
    }

    rtl_assert(
      variable->value.type == TEA_VALUE_INSTANCE, "Method can be called only from an object");

    tea_struct_declaration_t* struct_declaration =
      tea_find_struct_declaration(context, variable->value.object->type);
    rtl_assert(
      struct_declaration, "Can't find struct declaration %s", variable->value.object->type);

    // declare 'self' for the scope
    tea_declare_variable(context, &inner_scope, "self", 0, NULL, object_node);

    function_name = field_token->buffer;
    function = tea_context_find_function(&struct_declaration->functions, function_name);
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
    rtl_log_err("Can't find function %s", function_name);
    exit(1);
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
        rtl_log_err("Failed to allocate memory for variable %.*s", param_name_token->buffer_size,
          param_name_token->buffer);
        break;
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
        case TEA_VALUE_STRING:
          rtl_log_dbg("Declare param %s : %s = '%s'", param_name_token->buffer,
            tea_value_get_type_string(variable->value.type), variable->value.string);
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

static tea_value_t tea_interpret_evaluate_new(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* struct_name = node->token;
  if (!struct_name) {
    rtl_log_err("Invalid name!");
    exit(1);
  }

  const tea_struct_declaration_t* struct_declr =
    tea_find_struct_declaration(context, struct_name->buffer);

  tea_instance_t* object =
    rtl_malloc(sizeof(tea_instance_t) + struct_declr->field_count * sizeof(tea_value_t));
  if (!object) {
    rtl_log_err("Failed to allocate memory for object!");
    exit(1);
  }

  rtl_list_entry_t* field_entry = rtl_list_first(&struct_declr->node->children);
  rtl_list_entry_t* declr_entry;

  unsigned long field_index = 0;

  rtl_list_for_each(declr_entry, &node->children)
  {
    const tea_ast_node_t* declr_node = rtl_list_record(declr_entry, tea_ast_node_t, link);
    if (!declr_node) {
      rtl_log_err("Invalid declr_node!");
      exit(1);
    }

    const tea_ast_node_t* field_node = rtl_list_record(field_entry, tea_ast_node_t, link);

    const tea_ast_node_t* value_node = NULL;
    rtl_list_entry_t* first_child_entry = rtl_list_first(&declr_node->children);
    if (first_child_entry) {
      value_node = rtl_list_record(first_child_entry, tea_ast_node_t, link);
    } else {
      // Use the name of the field as the value,
      // so for example 'new Vec3 {x: x, y: y, z: z}' could be
      // shortened to 'new Vec3 {x, y, z}'
      value_node = declr_node;
    }

    if (!value_node) {
      rtl_log_err("Invalid value_node!");
      exit(1);
    }

    const tea_token_t* declr = declr_node->token;
    const tea_token_t* field = field_node->token;

    if (declr == NULL || field == NULL) {
      rtl_log_err("Invalid declr or field!");
      exit(1);
    }

    if (declr->buffer_size != field->buffer_size ||
        strncmp(declr->buffer, field->buffer, field->buffer_size) != 0) {
      rtl_log_err(
        "Field must be filled sequentially during struct init: %s (line: %d) and %s (line: %d)!",
        declr->buffer, declr->line, field->buffer, field->line);
      exit(1);
    }

    tea_value_t value_expr = tea_interpret_evaluate_expression(context, scope, value_node);
    memcpy(&object->buffer[field_index++ * sizeof(tea_value_t)], &value_expr, sizeof(tea_value_t));

    field_entry = rtl_list_next(field_entry, &struct_declr->node->children);
  }

  object->type = struct_name->buffer;

  tea_value_t result;
  result.type = TEA_VALUE_INSTANCE;
  result.object = object;

  return result;
}

static tea_value_t* tea_field_access(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node)
{
  // Extract field information
  tea_ast_node_t* field_node = node->field_access.field;
  rtl_assert(field_node, "Field access node missing field component in AST node");

  const tea_token_t* field_name = field_node->token;
  rtl_assert(field_name, "Field AST node missing token information");

  // Extract object information
  tea_ast_node_t* object_node = node->field_access.object;
  rtl_assert(object_node, "Field access node missing object component in AST node");

  const tea_token_t* object_name = object_node->token;
  rtl_assert(object_name, "Object AST node missing token information");

  // Get the variable containing the object
  const tea_variable_t* variable = tea_context_find_variable(scope, object_name->buffer);
  rtl_assert(variable, "Variable '%s' not found in current scope (line %d, col %d)",
    object_name->buffer, object_name->line, object_name->column);

  const tea_instance_t* object = variable->value.object;
  rtl_assert(variable->value.type == TEA_VALUE_INSTANCE,
    "Variable '%s' has type '%s' but expected object type (line %d, col %d)", object_name->buffer,
    tea_value_get_type_string(variable->value.type), object_name->line, object_name->column);

  // Find the struct declaration for this object type
  const tea_struct_declaration_t* struct_declr = tea_find_struct_declaration(context, object->type);
  rtl_assert(struct_declr,
    "Struct declaration not found for type '%s' when accessing field '%s' (line %d, col %d)",
    object->type, field_name->buffer, field_name->line, field_name->column);

  // Find the field by name and return its value
  unsigned long field_index = 0;
  rtl_list_entry_t* field_entry;

  rtl_list_for_each_indexed(field_index, field_entry, &struct_declr->node->children)
  {
    rtl_assert(field_index < struct_declr->field_count, "Field index out of range!");

    const tea_ast_node_t* field_decl_node = rtl_list_record(field_entry, tea_ast_node_t, link);
    rtl_assert(field_decl_node && field_decl_node->token, "Invalid field declaration node!");

    if (!strcmp(field_decl_node->token->buffer, field_name->buffer)) {
      return (tea_value_t*)&object->buffer[field_index * sizeof(tea_value_t)];
    }
  }

  return NULL;
}

static tea_value_t tea_interpret_field_access(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_value_t* result = tea_field_access(context, scope, node);
  if (result) {
    return *result;
  }

  return tea_value_invalid();
}

tea_value_t tea_interpret_evaluate_expression(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  rtl_assert(node, "Node can't be null");

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
        rtl_log_err("Failed to evaluate node <%s>, token: <%s> %.*s (line %d, col %d)",
          tea_ast_node_get_type_name(node->type), tea_token_get_name(token->type),
          token->buffer_size, token->buffer, token->line, token->column);
      } else {
        rtl_log_err("Failed to evaluate node <%s>", tea_ast_node_get_type_name(node->type));
      }
    } break;
  }

  exit(1);
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

#undef TEA_VARIABLE_POOL_ENABLED
