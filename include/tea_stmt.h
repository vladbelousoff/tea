#pragma once

#include "tea_fn.h"
#include "tea_scope.h"

typedef struct {
  unsigned char is_break_set : 1;
  unsigned char is_cont_set : 1;
} tea_loop_ctx_t;

bool tea_exec(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node,
              tea_ret_ctx_t *ret_ctx, tea_loop_ctx_t *loop_ctx);

bool tea_exec_stmt(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node,
                   tea_ret_ctx_t *ret_ctx, tea_loop_ctx_t *loop_ctx);

bool tea_exec_let(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node);

bool tea_exec_assign(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node);

bool tea_exec_if(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node,
                 tea_ret_ctx_t *ret_ctx, tea_loop_ctx_t *loop_ctx);

bool tea_exec_while(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node,
                    tea_ret_ctx_t *ret_ctx);

bool tea_exec_ret(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node,
                  tea_ret_ctx_t *ret_ctx);

bool tea_exec_break(tea_loop_ctx_t *loop_ctx);

bool tea_exec_cont(tea_loop_ctx_t *loop_ctx);
