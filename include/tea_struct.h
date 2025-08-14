#pragma once

#include "tea.h"
#include "tea_scope.h"
#include "tea_value.h"

typedef struct {
  tea_list_entry_t link;
  const tea_node_t *node;
  unsigned long field_cnt;
  tea_list_entry_t fns;
} tea_struct_decl_t;

bool tea_interp_struct_decl(tea_ctx_t *ctx, const tea_node_t *node);
tea_struct_decl_t *tea_find_struct_decl(const tea_ctx_t *ctx, const char *name);

bool tea_interp_impl_blk(const tea_ctx_t *ctx, const tea_node_t *node);

tea_val_t tea_eval_new(tea_ctx_t *ctx, tea_scope_t *scp,
                       const tea_node_t *node);

tea_val_t tea_eval_field_acc(const tea_ctx_t *ctx, const tea_scope_t *scp,
                             const tea_node_t *node);
tea_val_t *tea_get_field_ptr(const tea_ctx_t *ctx, const tea_scope_t *scp,
                             const tea_node_t *node);
