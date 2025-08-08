#pragma once

#include "tea_scope.h"
#include "tea_value.h"

typedef struct
{
  rtl_list_entry_t link;
  const tea_tok_t* name;
  const tea_tok_t* ret_type;
  const tea_node_t* body;
  const tea_node_t* params;
  unsigned char mut : 1;
} tea_fn_t;

typedef struct
{
  rtl_list_entry_t head;
} tea_fn_args_t;

typedef tea_val_t (*tea_native_fn_cb_t)(tea_ctx_t* ctx, const tea_fn_args_t* args);

typedef struct
{
  rtl_list_entry_t link;
  const char* name;
  tea_native_fn_cb_t cb;
} tea_native_fn_t;

typedef struct
{
  tea_val_t ret_val;
  bool is_set;
} tea_ret_ctx_t;

tea_var_t* tea_fn_args_pop(const tea_fn_args_t* args);

const tea_native_fn_t* tea_ctx_find_native_fn(const rtl_list_entry_t* fns, const char* name);
const tea_fn_t* tea_ctx_find_fn(const rtl_list_entry_t* fns, const char* name);

bool tea_decl_fn(const tea_node_t* node, rtl_list_entry_t* fns);
bool tea_interp_fn_decl(tea_ctx_t* ctx, const tea_node_t* node);

tea_val_t tea_eval_fn_call(tea_ctx_t* ctx, tea_scope_t* scp, const tea_node_t* node);
tea_val_t tea_eval_native_fn_call(
  tea_ctx_t* ctx, tea_scope_t* scp, const tea_native_fn_t* nfn, const tea_node_t* args);

void tea_bind_native_fn(tea_ctx_t* ctx, const char* name, tea_native_fn_cb_t cb);
