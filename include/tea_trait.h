#pragma once

#include "rtl.h"
#include "tea_ast.h"
#include "tea_fn.h"
#include "tea_scope.h"

typedef struct {
  rtl_list_entry_t link;
  const tea_node_t *node;
  const char *name;
  rtl_list_entry_t methods;
} tea_trait_decl_t;

typedef struct {
  rtl_list_entry_t link;
  const tea_node_t *node;
  const char *trait_name;
  const char *struct_name;
  rtl_list_entry_t methods;
} tea_trait_impl_t;

bool tea_interp_trait_decl(tea_ctx_t *ctx, const tea_node_t *node);
tea_trait_decl_t *tea_find_trait_decl(const tea_ctx_t *ctx, const char *name);

bool tea_interp_trait_impl(tea_ctx_t *ctx, const tea_node_t *node);
tea_trait_impl_t *tea_find_trait_impl(const tea_ctx_t *ctx,
                                      const char *trait_name,
                                      const char *struct_name);

const tea_fn_t *tea_resolve_trait_method(const tea_ctx_t *ctx,
                                         const char *struct_name,
                                         const char *method_name);
