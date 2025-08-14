#pragma once

#include <rtl_list.h>

#include "tea_token.h"

typedef enum {
  TEA_N_PROG,
  TEA_N_FN,
  TEA_N_RET_TYPE,
  TEA_N_PARAM,
  TEA_N_ATTR,
  TEA_N_STMT,
  TEA_N_LET,
  TEA_N_MUT,
  TEA_N_ASSIGN,
  TEA_N_BINOP,
  TEA_N_UNARY,
  TEA_N_IDENT,
  TEA_N_TYPE_ANNOT,
  TEA_N_INT,
  TEA_N_FLOAT,
  TEA_N_FN_CALL,
  TEA_N_FN_ARGS,
  TEA_N_RET,
  TEA_N_BREAK,
  TEA_N_CONT,
  TEA_N_IF,
  TEA_N_THEN,
  TEA_N_ELSE,
  TEA_N_WHILE,
  TEA_N_WHILE_COND,
  TEA_N_WHILE_BODY,
  TEA_N_STRUCT,
  TEA_N_STRUCT_FIELD,
  TEA_N_STRUCT_INST,
  TEA_N_STRUCT_INIT,
  TEA_N_IMPL_ITEM,
  TEA_N_IMPL_BLK,
  TEA_N_STR,
  TEA_N_FIELD_ACC,
  TEA_N_OPT_TYPE,
  TEA_N_NULL,

} tea_node_type_t;

typedef struct tea_node {
  rtl_list_entry_t link;
  tea_node_type_t type;
  tea_tok_t *tok;

  union {
    rtl_list_entry_t children;

    struct {
      struct tea_node *lhs;
      struct tea_node *rhs;
    } binop;

    struct {
      struct tea_node *obj;
      struct tea_node *field;
    } field_acc;
  };
} tea_node_t;

#define tea_node_create(type, token)                                           \
  _tea_node_create(__FILE__, __LINE__, type, token)

tea_node_t *_tea_node_create(const char *file, unsigned long line,
                             tea_node_type_t type, tea_tok_t *tok);

tea_node_t *tea_node_create_int(const char *file, unsigned long line,
                                tea_tok_t *tok);

tea_node_t *tea_node_create_float(const char *file, unsigned long line,
                                  tea_tok_t *tok);

void tea_node_add_child(tea_node_t *parent, tea_node_t *child);
void tea_node_add_children(tea_node_t *parent,
                           const rtl_list_entry_t *children);
void tea_node_set_binop(tea_node_t *parent, tea_node_t *lhs, tea_node_t *rhs);
void tea_node_set_field_acc(tea_node_t *parent, tea_node_t *obj,
                            tea_node_t *field);
void tea_node_free(tea_node_t *node);
void tea_node_print(tea_node_t *node, int depth);

const char *tea_node_type_name(tea_node_type_t type);
