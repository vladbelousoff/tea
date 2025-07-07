#pragma once

#include <rtl_list.h>

#include "tea_token.h"

typedef enum
{
  TEA_AST_NODE_PROGRAM,
  TEA_AST_NODE_FUNCTION,
  TEA_AST_NODE_NATIVE_FUNCTION,
  TEA_AST_NODE_PARAM,
  TEA_AST_NODE_ATTR,
  TEA_AST_NODE_STMT,
  TEA_AST_NODE_LET,
  TEA_AST_NODE_LET_MUT,
  TEA_AST_NODE_ASSIGN,
  TEA_AST_NODE_BINOP,
  TEA_AST_NODE_UNARY,
  TEA_AST_NODE_IDENT,
  TEA_AST_NODE_NUMBER,
  TEA_AST_NODE_CALL,
  TEA_AST_NODE_RETURN,
  TEA_AST_NODE_IF,
  TEA_AST_NODE_THEN,
  TEA_AST_NODE_ELSE,
} tea_ast_node_type_t;

typedef struct tea_ast_node
{
  rtl_list_entry_t link;
  tea_ast_node_type_t type;
  tea_token_t* token;

  union
  {
    rtl_list_entry_t children;

    struct
    {
      struct tea_ast_node* lhs;
      struct tea_ast_node* rhs;
    } binop;
  };
} tea_ast_node_t;

#define tea_ast_node_create(type, token) _tea_ast_node_create(__FILE__, __LINE__, type, token)

tea_ast_node_t* _tea_ast_node_create(
  const char* file, unsigned long line, tea_ast_node_type_t type, tea_token_t* token);
void tea_ast_node_add_child(tea_ast_node_t* parent, tea_ast_node_t* child);
void tea_ast_node_add_children(tea_ast_node_t* parent, const rtl_list_entry_t* children);
void tea_ast_node_set_binop_children(
  tea_ast_node_t* parent, tea_ast_node_t* lhs, tea_ast_node_t* rhs);
void tea_ast_node_free(tea_ast_node_t* node);
void tea_ast_node_print(tea_ast_node_t* node, int depth);
