#pragma once

#include <rtl_list.h>

#include "tea_token.h"

typedef enum
{
  TEA_AST_NODE_PROGRAM,
  TEA_AST_NODE_FUNCTION,
  TEA_AST_NODE_PARAM,
  TEA_AST_NODE_ATTR
} tea_ast_node_type_t;

typedef struct
{
  rtl_list_entry_t link;
  rtl_list_entry_t children;
  tea_ast_node_type_t type;
  tea_token_t* token;
} tea_ast_node_t;

tea_ast_node_t* tea_ast_node_create(tea_ast_node_type_t type, tea_token_t* token);
void tea_ast_node_add_child(tea_ast_node_t* parent, tea_ast_node_t* child);
void tea_ast_node_add_children(tea_ast_node_t* parent, const rtl_list_entry_t* children);
void tea_ast_node_free(tea_ast_node_t* node);
void tea_ast_node_print(tea_ast_node_t* node, int depth);
