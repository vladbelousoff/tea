#include "tea_expr.h"

#include "tea_fn.h"
#include "tea_struct.h"

#include <stdlib.h>

#include <rtl.h>
#include <rtl_log.h>

#include "tea_grammar.h"

tea_val_t tea_eval_int(tea_tok_t *token)
{
  tea_val_t value;
  value.type = TEA_V_I32;
  value.i32 = *(int *)&token->buf;

  return value;
}

tea_val_t tea_eval_float(tea_tok_t *token)
{
  tea_val_t value;
  value.type = TEA_V_F32;
  value.f32 = *(float *)&token->buf;

  return value;
}

tea_val_t tea_eval_binop(tea_ctx_t *ctx, tea_scope_t *scp,
                         const tea_node_t *node)
{
  const tea_val_t lhs_val = tea_eval_expr(ctx, scp, node->binop.lhs);
  const tea_val_t rhs_val = tea_eval_expr(ctx, scp, node->binop.rhs);

  return tea_val_binop(lhs_val, rhs_val, node->tok);
}

tea_val_t tea_eval_unary(tea_ctx_t *ctx, tea_scope_t *scp,
                         const tea_node_t *node)
{
  rtl_list_entry_t *entry = rtl_list_first(&node->children);
  const tea_node_t *operand = rtl_list_record(entry, tea_node_t, link);
  tea_val_t operand_val = tea_eval_expr(ctx, scp, operand);
  const tea_tok_t *token = node->tok;

  switch (token->type) {
  case TEA_TOKEN_PLUS:
    break;
  case TEA_TOKEN_MINUS:
    switch (operand_val.type) {
    case TEA_V_I32:
      operand_val.i32 = -operand_val.i32;
      break;
    case TEA_V_F32:
      operand_val.f32 = -operand_val.f32;
      break;
    default:
      break;
    }
    break;
  case TEA_TOKEN_EXCLAMATION_MARK:
    switch (operand_val.type) {
    case TEA_V_I32:
      operand_val.i32 = !operand_val.i32;
      break;
    default:
      break;
    }
    break;
  default:
    rtl_log_err(
      "Expression evaluation error: Invalid unary operator '%s' at line %d, column %d",
      token->buf, token->line, token->col);
    break;
  }

  // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
  return operand_val;
}

tea_val_t tea_eval_ident(const tea_scope_t *scp, const tea_node_t *node)
{
  const tea_tok_t *token = node->tok;
  if (!token) {
    rtl_log_err(
      "Internal error: Missing token for identifier node during expression evaluation");
    return tea_val_undef();
  }

  const tea_var_t *variable = tea_scope_find(scp, token->buf);
  if (!variable) {
    rtl_log_err(
      "Runtime error: Undefined variable '%s' referenced at line %d, column %d",
      token->buf, token->line, token->col);
    return tea_val_undef();
  }

  return variable->val;
}

tea_val_t tea_interpret_evaluate_string(const tea_node_t *node)
{
  const tea_tok_t *token = node->tok;
  if (!token) {
    rtl_log_err(
      "Internal error: Missing token for string literal node during expression evaluation");
    return tea_val_undef();
  }

  tea_inst_t *object = rtl_malloc(sizeof(tea_inst_t) + token->size + 1);
  if (!object) {
    rtl_log_err(
      "Memory error: Failed to allocate memory for struct '%s' instance at line %d, column %d",
      token->buf, token->line, token->col);
    return tea_val_undef();
  }

  object->type = "string";
  memcpy(object->buf, token->buf, token->size + 1);

  const tea_val_t result = { .type = TEA_V_INST, .obj = object };
  return result;
}

tea_val_t tea_eval_expr(tea_ctx_t *ctx, tea_scope_t *scp,
                        const tea_node_t *node)
{
  if (!node) {
    rtl_log_err("Internal error: Null AST node passed to expression evaluator");
    return tea_val_undef();
  }

  switch (node->type) {
  case TEA_N_INT:
    return tea_eval_int(node->tok);
  case TEA_N_FLOAT:
    return tea_eval_float(node->tok);
  case TEA_N_BINOP:
    return tea_eval_binop(ctx, scp, node);
  case TEA_N_UNARY:
    return tea_eval_unary(ctx, scp, node);
  case TEA_N_IDENT:
    return tea_eval_ident(scp, node);
  case TEA_N_STR:
    return tea_interpret_evaluate_string(node);
  case TEA_N_FN_CALL:
    return tea_eval_fn_call(ctx, scp, node);
  case TEA_N_STRUCT_INST:
    return tea_eval_new(ctx, scp, node);
  case TEA_N_FIELD_ACC:
    return tea_eval_field_acc(ctx, scp, node);
  case TEA_N_NULL:
    return tea_val_null();
  default: {
    tea_tok_t *token = node->tok;
    if (token) {
      rtl_log_err(
        "Expression evaluation error: Unsupported node type <%s> with token <%s> '%.*s' at line "
        "%d, column %d",
        tea_node_type_name(node->type), tea_tok_name(token->type), token->size,
        token->buf, token->line, token->col);
    } else {
      rtl_log_err(
        "Expression evaluation error: Unsupported node type <%s> (no token available)",
        tea_node_type_name(node->type));
    }
  } break;
  }

  return tea_val_undef();
}
