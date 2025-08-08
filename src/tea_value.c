#include "tea_value.h"

#include <stdlib.h>
#include <string.h>

#include <rtl.h>
#include <rtl_log.h>

#include "tea_grammar.h"

const char* tea_val_type_str(const tea_val_type_t type)
{
  switch (type) {
    case TEA_V_UNDEF:
      return "unset";
    case TEA_V_I32:
      return "i32";
    case TEA_V_F32:
      return "f32";
    case TEA_V_INST:
      return "object";
    case TEA_V_NULL:
      return "null";
  }

  return "UNKNOWN";
}

typedef struct
{
  const char* name;
  tea_val_type_t type;
} tea_type_string_id;

tea_val_type_t tea_value_get_type_by_string(const char* name)
{
  static tea_type_string_id ids[] = { { "i32", TEA_V_I32 }, { "f32", TEA_V_F32 },
    { "string", TEA_V_INST }, { NULL, TEA_V_UNDEF } };

  for (int i = 0; ids[i].name; ++i) {
    if (!strcmp(name, ids[i].name)) {
      return ids[i].type;
    }
  }

  return TEA_V_UNDEF;
}

tea_val_t tea_val_undef()
{
  static tea_val_t result = { .type = TEA_V_UNDEF };
  return result;
}

tea_val_t tea_val_null()
{
  // For the keyword 'null' we don't know the exact type, so let it be just null
  static tea_val_t result = { .type = TEA_V_NULL, .null_type = TEA_V_NULL };
  return result;
}

#define TEA_APPLY_BINOP(a, b, op, result)                                                          \
  do {                                                                                             \
    switch (op->type) {                                                                            \
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
          rtl_log_err("Runtime error: Division by zero at line %d, column %d", op->line, op->col); \
          return tea_val_undef();                                                                \
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

tea_val_t tea_val_binop(const tea_val_t lhs_val, const tea_val_t rhs_val, const tea_tok_t* op)
{
  tea_val_t result;

  // By-default it's i32 for boolean ops
  result.type = TEA_V_I32;

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

  if (lhs_val.type == TEA_V_I32) {
    if (rhs_val.type == TEA_V_I32) {
      result.type = TEA_V_I32;
      TEA_APPLY_BINOP(lhs_val.i32, rhs_val.i32, op, result.i32);
      return result;
    }
    if (rhs_val.type == TEA_V_F32) {
      result.type = TEA_V_F32;
      TEA_APPLY_BINOP(lhs_val.i32, rhs_val.f32, op, result.f32);
      return result;
    }
  }

  if (lhs_val.type == TEA_V_F32) {
    if (rhs_val.type == TEA_V_I32) {
      result.type = TEA_V_F32;
      TEA_APPLY_BINOP(lhs_val.f32, rhs_val.i32, op, result.f32);
      return result;
    }
    if (rhs_val.type == TEA_V_F32) {
      result.type = TEA_V_F32;
      TEA_APPLY_BINOP(lhs_val.f32, rhs_val.f32, op, result.f32);
      return result;
    }
  }

  rtl_log_err(
    "Runtime error: Unsupported binary operation '%s' between types %s and %s at line %d, column "
    "%d",
    tea_tok_name(op->type), tea_val_type_str(lhs_val.type), tea_val_type_str(rhs_val.type),
    op->line, op->col);
  return tea_val_undef();
}

#undef TEA_APPLY_BINOP
