#pragma once

#include <rtl_list.h>

#include "tea_token.h"

typedef enum
{
  TEA_AST_NODE_PROGRAM,
  TEA_AST_NODE_FUNCTION,
  TEA_AST_NODE_RETURN_TYPE,
  TEA_AST_NODE_PARAM,
  TEA_AST_NODE_ATTR,
  TEA_AST_NODE_STMT,
  TEA_AST_NODE_LET,
  TEA_AST_NODE_MUT,
  TEA_AST_NODE_ASSIGN,
  TEA_AST_NODE_BINOP,
  TEA_AST_NODE_UNARY,
  TEA_AST_NODE_IDENT,
  TEA_AST_NODE_TYPE_ANNOT,
  TEA_AST_NODE_INTEGER_NUMBER,
  TEA_AST_NODE_FLOAT_NUMBER,
  TEA_AST_NODE_FUNCTION_CALL,
  TEA_AST_NODE_FUNCTION_CALL_ARGS,
  TEA_AST_NODE_RETURN,
  TEA_AST_NODE_BREAK,
  TEA_AST_NODE_CONTINUE,
  TEA_AST_NODE_IF,
  TEA_AST_NODE_THEN,
  TEA_AST_NODE_ELSE,
  TEA_AST_NODE_WHILE,
  TEA_AST_NODE_WHILE_COND,
  TEA_AST_NODE_WHILE_BODY,
  TEA_AST_NODE_STRUCT,
  TEA_AST_NODE_STRUCT_FIELD,
  TEA_AST_NODE_STRUCT_INSTANCE,
  TEA_AST_NODE_STRUCT_FIELD_INIT,
  TEA_AST_NODE_IMPL_ITEM,
  TEA_AST_NODE_IMPL_BLOCK,
  TEA_AST_NODE_STRING,
  TEA_AST_NODE_FIELD_ACCESS,
  TEA_AST_NODE_OPTIONAL_TYPE,
  TEA_AST_NODE_NULL,
  TEA_AST_NODE_TRAIT,
  TEA_AST_NODE_TRAIT_METHOD,
  TEA_AST_NODE_TRAIT_IMPL,
  TEA_AST_NODE_TRAIT_IMPL_ITEM,
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

    struct
    {
      struct tea_ast_node* object;
      struct tea_ast_node* field;
    } field_access;
  };
} tea_ast_node_t;

#define tea_ast_node_create(type, token) _tea_ast_node_create(__FILE__, __LINE__, type, token)

tea_ast_node_t* _tea_ast_node_create(
  const char* file, unsigned long line, tea_ast_node_type_t type, tea_token_t* token);

tea_ast_node_t* tea_ast_node_create_integer_number(
  const char* file, unsigned long line, tea_token_t* token);

tea_ast_node_t* tea_ast_node_create_float_number(
  const char* file, unsigned long line, tea_token_t* token);

void tea_ast_node_add_child(tea_ast_node_t* parent, tea_ast_node_t* child);
void tea_ast_node_add_children(tea_ast_node_t* parent, const rtl_list_entry_t* children);
void tea_ast_node_set_binop_children(
  tea_ast_node_t* parent, tea_ast_node_t* lhs, tea_ast_node_t* rhs);
void tea_ast_node_set_field_access_children(
  tea_ast_node_t* parent, tea_ast_node_t* object, tea_ast_node_t* field);
void tea_ast_node_free(tea_ast_node_t* node);
void tea_ast_node_print(tea_ast_node_t* node, int depth);

const char* tea_ast_node_get_type_name(tea_ast_node_type_t type);
