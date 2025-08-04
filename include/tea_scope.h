#pragma once

#include "tea_ast.h"
#include "tea_value.h"

typedef struct
{
  const char* filename;
  rtl_list_entry_t functions;
  rtl_list_entry_t native_functions;
  rtl_list_entry_t struct_declarations;
  rtl_list_entry_t trait_declarations;
  rtl_list_entry_t trait_implementations;
  rtl_list_entry_t variable_pool;
} tea_context_t;

#define TEA_VARIABLE_MUTABLE 1 << 0

typedef struct
{
  rtl_list_entry_t link;
  const char* name;
  tea_value_t value;
  unsigned char flags;
} tea_variable_t;

typedef struct tea_scope_t
{
  struct tea_scope_t* parent_scope;
  rtl_list_entry_t variables;
} tea_scope_t;

tea_variable_t* tea_allocate_variable(const tea_context_t* context);
void tea_free_variable(tea_context_t* context, tea_variable_t* variable);

void tea_scope_init(tea_scope_t* scope, tea_scope_t* parent_scope);
void tea_scope_cleanup(tea_context_t* context, const tea_scope_t* scope);

tea_variable_t* tea_scope_find_variable_local(const tea_scope_t* scope, const char* name);
tea_variable_t* tea_scope_find_variable(const tea_scope_t* scope, const char* name);

bool tea_declare_variable(tea_context_t* context, tea_scope_t* scope, const char* name,
  unsigned int flags, const char* type, const tea_ast_node_t* initial_value);
