#pragma once

#include "tea_ast.h"

#include <stdint.h>

typedef enum
{
  TEA_VALUE_NONE,
  TEA_VALUE_I32,
  TEA_VALUE_F32,
  TEA_VALUE_STRING,
  TEA_VALUE_INSTANCE,
} tea_value_type_t;

const char* tea_value_get_type_string(tea_value_type_t type);

typedef struct
{
  const char* type;
  unsigned long buffer_size;
  char buffer[0];
} tea_instance_t;

typedef struct
{
  tea_value_type_t type;

  union
  {
    float f32;
    int32_t i32;
    const char* string;
    tea_instance_t* object;
  };
} tea_value_t;

typedef struct tea_scope
{
  struct tea_scope* parent_scope;
  rtl_list_entry_t variables;
} tea_scope_t;

typedef struct
{
  const char* filename;
  rtl_list_entry_t functions;
  rtl_list_entry_t native_functions;
  rtl_list_entry_t struct_declarations;
  rtl_list_entry_t variable_pool;
} tea_context_t;

typedef struct
{
  rtl_list_entry_t link;
  const char* name;
  tea_value_t value;
  unsigned char is_mutable : 1;
} tea_variable_t;

typedef struct
{
  rtl_list_entry_t link;
  const tea_token_t* name;
  const tea_token_t* return_type;
  const tea_ast_node_t* body;
  const tea_ast_node_t* params;
  unsigned char is_mutable : 1;
} tea_function_t;

typedef tea_value_t (*tea_native_function_cb_t)(const tea_value_t* args, int arg_count);

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

typedef struct
{
  unsigned char is_break_set : 1;
  unsigned char is_continue_set : 1;
} tea_loop_context_t;

typedef struct
{
  rtl_list_entry_t link;
  const tea_ast_node_t* node;
  unsigned long field_count;
} tea_struct_declaration_t;

tea_value_t tea_value_none();

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent_scope);
void tea_scope_cleanup(tea_context_t* context, const tea_scope_t* scope);

void tea_interpret_init(tea_context_t* context, const char* filename);
void tea_interpret_cleanup(const tea_context_t* context);
bool tea_interpret_execute(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node,
  tea_return_context_t* return_context, tea_loop_context_t* break_context);
tea_value_t tea_interpret_evaluate_expression(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);

void tea_bind_native_function(
  tea_context_t* context, const char* name, tea_native_function_cb_t cb);
