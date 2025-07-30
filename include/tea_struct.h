#pragma once

#include "rtl.h"
#include "tea_scope.h"
#include "tea_value.h"

typedef struct
{
  rtl_list_entry_t link;
  const tea_ast_node_t* node;
  unsigned long field_count;
  rtl_list_entry_t functions;
} tea_struct_declaration_t;

bool tea_interpret_struct_declaration(tea_context_t* context, const tea_ast_node_t* node);
tea_struct_declaration_t* tea_find_struct_declaration(
  const tea_context_t* context, const char* name);

bool tea_interpret_impl_block(const tea_context_t* context, const tea_ast_node_t* node);

tea_value_t tea_interpret_evaluate_new(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);

tea_value_t tea_interpret_field_access(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node);
tea_value_t* tea_get_field_pointer(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node);
