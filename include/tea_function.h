#pragma once

#include "tea_scope.h"
#include "tea_value.h"

// Context type is defined in tea_scope.h

typedef struct
{
  rtl_list_entry_t link;
  const tea_token_t* name;
  const tea_token_t* return_type;
  const tea_ast_node_t* body;
  const tea_ast_node_t* params;
  unsigned char mut : 1;
} tea_function_t;

typedef struct
{
  rtl_list_entry_t list_head;
} tea_function_args_t;

typedef tea_value_t (*tea_native_function_cb_t)(
  tea_context_t* context, const tea_function_args_t* args);

typedef struct
{
  rtl_list_entry_t link;
  const char* name;
  tea_native_function_cb_t cb;
} tea_native_function_t;

typedef struct
{
  tea_value_t returned_value;
  bool is_set;
} tea_return_context_t;

// Function argument management
tea_variable_t* tea_function_args_pop(const tea_function_args_t* args);

// Function lookup
const tea_native_function_t* tea_context_find_native_function(
  const rtl_list_entry_t* functions, const char* name);
const tea_function_t* tea_context_find_function(
  const rtl_list_entry_t* functions, const char* name);

// Function declaration and execution
bool tea_declare_function(const tea_ast_node_t* node, rtl_list_entry_t* functions);
bool tea_interpret_function_declaration(tea_context_t* context, const tea_ast_node_t* node);

// Function call evaluation
tea_value_t tea_interpret_evaluate_function_call(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);
tea_value_t tea_interpret_evaluate_native_function_call(tea_context_t* context, tea_scope_t* scope,
  const tea_native_function_t* native_function, const tea_ast_node_t* function_call_args);

// Native function binding
void tea_bind_native_function(
  tea_context_t* context, const char* name, tea_native_function_cb_t cb);
