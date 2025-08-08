#pragma once

#include <stdint.h>

#include "tea_token.h"

typedef enum {
  TEA_V_UNDEF,
  TEA_V_NULL,
  TEA_V_I32,
  TEA_V_F32,
  TEA_V_INST,
} tea_val_type_t;

typedef struct {
  const char *type;
  unsigned long size;
  char buf[0];
} tea_inst_t;

typedef struct {
  tea_val_type_t type;

  union {
    float f32;
    int32_t i32;
    tea_inst_t *obj;
    // if the type is TEA_V_NULL,
    // we need to know the exact type of the null
    tea_val_type_t null_type;
  };
} tea_val_t;

const char *tea_val_type_str(tea_val_type_t type);
tea_val_type_t tea_val_type_by_str(const char *name);

tea_val_t tea_val_undef();
tea_val_t tea_val_null();

tea_val_t tea_val_binop(tea_val_t lhs, tea_val_t rhs, const tea_tok_t *op);
