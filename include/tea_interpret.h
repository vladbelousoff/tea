#pragma once

#include "tea_ast.h"

typedef struct
{
  rtl_list_entry_t variables;
} tea_context_t;

typedef enum
{
  TEA_VALUE_I32,
  TEA_VALUE_F32,
  TEA_VALUE_STRING,
  TEA_VALUE_OBJECT,
} tea_value_type_t;

const char* tea_value_get_type_string(tea_value_type_t type);

typedef struct
{
  tea_value_type_t type : 2;

  union
  {
    int i32_value;
    float f32_value;
    char* string_value;
    void* object_value;
  };
} tea_value_t;

typedef struct
{
  rtl_list_entry_t link;
  const tea_token_t* name;
  tea_value_t value;
  unsigned char is_mutable : 1;
} tea_variable_t;

void tea_interpret_init(tea_context_t* context);
bool tea_interpret_execute(tea_context_t* context, const tea_ast_node_t* node);
tea_value_t tea_interpret_evaluate_expression(tea_context_t* context, const tea_ast_node_t* node);
