#pragma once

#include "rtl.h"
#include "tea_ast.h"
#include "tea_function.h"
#include "tea_scope.h"

typedef struct
{
  rtl_list_entry_t link;
  const tea_ast_node_t* node;
  const char* name;
  rtl_list_entry_t methods;
} tea_trait_declaration_t;

typedef struct
{
  rtl_list_entry_t link;
  const tea_ast_node_t* node;
  const char* trait_name;
  const char* struct_name;
  rtl_list_entry_t methods;
} tea_trait_implementation_t;

bool tea_interpret_trait_declaration(tea_context_t* context, const tea_ast_node_t* node);
tea_trait_declaration_t* tea_find_trait_declaration(const tea_context_t* context, const char* name);

bool tea_interpret_trait_implementation(tea_context_t* context, const tea_ast_node_t* node);
tea_trait_implementation_t* tea_find_trait_implementation(
  const tea_context_t* context, const char* trait_name, const char* struct_name);

const tea_function_t* tea_resolve_trait_method(
  const tea_context_t* context, const char* struct_name, const char* method_name);
