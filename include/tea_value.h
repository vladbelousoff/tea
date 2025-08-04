#pragma once

#include <stdint.h>

#include "tea_token.h"

typedef enum
{
  TEA_VALUE_INVALID,
  TEA_VALUE_NULL,
  TEA_VALUE_I32,
  TEA_VALUE_F32,
  TEA_VALUE_INSTANCE,
} tea_value_type_t;

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
    tea_instance_t* object;
    // if the type is TEA_VALUE_NULL,
    // we need to know the exact type of the null
    tea_value_type_t null_type;
  };
} tea_value_t;

const char* tea_value_get_type_string(tea_value_type_t type);
tea_value_type_t tea_value_get_type_by_string(const char* name);

tea_value_t tea_value_invalid();
tea_value_t tea_value_null();

tea_value_t tea_value_binop(tea_value_t lhs_val, tea_value_t rhs_val, const tea_token_t* op);
