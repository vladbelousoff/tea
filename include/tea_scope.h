#pragma once

#include "tea_ast.h"
#include "tea_value.h"

typedef struct {
  const char *file_name;
  tea_list_entry_t funcs;
  tea_list_entry_t native_funcs;
  tea_list_entry_t structs;
  tea_list_entry_t vars;
} tea_ctx_t;

#define TEA_VAR_MUT 1 << 0
#define TEA_VAR_OPT 1 << 1

typedef struct {
  tea_list_entry_t link;
  const char *name;
  tea_val_t val;
  unsigned char flags;
} tea_var_t;

typedef struct tea_scope_t {
  struct tea_scope_t *parent;
  tea_list_entry_t vars;
} tea_scope_t;

tea_var_t *tea_alloc_var(const tea_ctx_t *ctx);
void tea_free_var(tea_ctx_t *ctx, tea_var_t *var);

void tea_scope_init(tea_scope_t *scp, tea_scope_t *parent);
void tea_scope_cleanup(tea_ctx_t *ctx, const tea_scope_t *scp);

tea_var_t *tea_scope_find_local(const tea_scope_t *scp, const char *name);
tea_var_t *tea_scope_find(const tea_scope_t *scp, const char *name);

bool tea_decl_var(tea_ctx_t *ctx, tea_scope_t *scp, const char *name,
                  unsigned int flags, const char *type, const tea_node_t *initial_value);
