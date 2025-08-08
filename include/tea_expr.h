#pragma once

#include "tea_scope.h"
#include "tea_value.h"

tea_val_t tea_eval_expr(tea_ctx_t* ctx, tea_scope_t* scp, const tea_node_t* node);

tea_val_t tea_eval_int(tea_tok_t* tok);
tea_val_t tea_eval_float(tea_tok_t* tok);
tea_val_t tea_eval_binop(tea_ctx_t* ctx, tea_scope_t* scp, const tea_node_t* node);
tea_val_t tea_eval_unary(tea_ctx_t* ctx, tea_scope_t* scp, const tea_node_t* node);
tea_val_t tea_eval_ident(const tea_scope_t* scp, const tea_node_t* node);
tea_val_t tea_eval_str(const tea_node_t* node);
