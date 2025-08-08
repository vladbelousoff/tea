#include "tea_ast.h"

#include "rtl_memory.h"

#include <rtl_log.h>
#include <string.h>

tea_node_t *_tea_node_create(const char *file, const unsigned long line,
                             const tea_node_type_t type, tea_tok_t *token)
{
  tea_node_t *node = _rtl_malloc(file, line, sizeof(*node));
  if (!node) {
    rtl_log_err("Critical error: Failed to allocate memory for AST node");
    return NULL;
  }

  node->type = type;
  node->tok = token;

  if (type == TEA_N_BINOP || type == TEA_N_ASSIGN) {
    node->binop.lhs = NULL;
    node->binop.rhs = NULL;
  } else if (type == TEA_N_FIELD_ACC) {
    node->field_acc.obj = NULL;
    node->field_acc.field = NULL;
  } else {
    rtl_list_init(&node->children);
  }

  return node;
}

void tea_node_add_child(tea_node_t *parent, tea_node_t *child)
{
  if (parent && child) {
    rtl_list_add_tail(&parent->children, &child->link);
  }
}

void tea_node_add_children(tea_node_t *parent, const rtl_list_entry_t *children)
{
  rtl_list_entry_t *entry;
  rtl_list_entry_t *safe;
  rtl_list_for_each_safe(entry, safe, children)
  {
    tea_node_t *child = rtl_list_record(entry, tea_node_t, link);
    rtl_list_remove(entry);
    tea_node_add_child(parent, child);
  }
}

void tea_node_free(tea_node_t *node)
{
  if (!node) {
    return;
  }

  if (node->type == TEA_N_BINOP || node->type == TEA_N_ASSIGN) {
    if (node->binop.lhs) {
      tea_node_free(node->binop.lhs);
    }
    if (node->binop.rhs) {
      tea_node_free(node->binop.rhs);
    }
  } else if (node->type == TEA_N_FIELD_ACC) {
    if (node->field_acc.obj) {
      tea_node_free(node->field_acc.obj);
    }
    if (node->field_acc.field) {
      tea_node_free(node->field_acc.field);
    }
  } else {
    rtl_list_entry_t *entry;
    rtl_list_entry_t *safe;
    rtl_list_for_each_safe(entry, safe, &node->children)
    {
      tea_node_t *child = rtl_list_record(entry, tea_node_t, link);
      rtl_list_remove(entry);
      tea_node_free(child);
    }
  }

  rtl_free(node);
}

const char *tea_node_type_name(const tea_node_type_t type)
{
  switch (type) {
  case TEA_N_PROG:
    return "PROGRAM";
  case TEA_N_FN:
    return "FUNCTION";
  case TEA_N_RET_TYPE:
    return "RETURN_TYPE";
  case TEA_N_PARAM:
    return "PARAM";
  case TEA_N_ATTR:
    return "ATTR";
  case TEA_N_STMT:
    return "STMT";
  case TEA_N_LET:
    return "LET";
  case TEA_N_MUT:
    return "MUT";
  case TEA_N_ASSIGN:
    return "ASSIGN";
  case TEA_N_BINOP:
    return "BINOP";
  case TEA_N_UNARY:
    return "UNARY";
  case TEA_N_IDENT:
    return "IDENT";
  case TEA_N_TYPE_ANNOT:
    return "TYPE_ANNOT";
  case TEA_N_INT:
    return "INTEGER";
  case TEA_N_FLOAT:
    return "FLOAT";
  case TEA_N_FN_CALL:
    return "FUNCTION_CALL";
  case TEA_N_FN_ARGS:
    return "FUNCTION_CALL_ARGS";
  case TEA_N_RET:
    return "RETURN";
  case TEA_N_BREAK:
    return "BREAK";
  case TEA_N_CONT:
    return "CONTINUE";
  case TEA_N_IF:
    return "IF";
  case TEA_N_THEN:
    return "THEN";
  case TEA_N_ELSE:
    return "ELSE";
  case TEA_N_WHILE:
    return "WHILE";
  case TEA_N_WHILE_COND:
    return "WHILE_COND";
  case TEA_N_WHILE_BODY:
    return "WHILE_BODY";
  case TEA_N_STRUCT:
    return "STRUCT";
  case TEA_N_STRUCT_FIELD:
    return "STRUCT_FIELD";
  case TEA_N_STRUCT_INST:
    return "STRUCT_INSTANCE";
  case TEA_N_STRUCT_INIT:
    return "STRUCT_FIELD_INIT";
  case TEA_N_STR:
    return "STRING";
  case TEA_N_FIELD_ACC:
    return "FIELD_ACCESS";
  case TEA_N_IMPL_ITEM:
    return "IMPL_ITEM";
  case TEA_N_IMPL_BLK:
    return "IMPL_BLOCK";
  case TEA_N_OPT_TYPE:
    return "OPTIONAL_TYPE";
  case TEA_N_NULL:
    return "NULL";
  case TEA_N_TRAIT:
    return "TRAIT";
  case TEA_N_TRAIT_METHOD:
    return "TRAIT_METHOD";
  case TEA_N_TRAIT_IMPL:
    return "TRAIT_IMPL";
  case TEA_N_TRAIT_IMPL_ITEM:
    return "TRAIT_IMPL_ITEM";
  default:
    return "UNKNOWN";
  }
}

#ifdef TEA_DEBUG_BUILD

static void print_tree_prefix(const int depth)
{
  for (int i = 0; i < depth; i++) {
    if (i == depth - 1) {
      printf("`- ");
    } else {
      printf("|  ");
    }
  }
}

static void tea_node_print_tree_recursive(tea_node_t *node, const int depth)
{
  if (!node) {
    return;
  }

  print_tree_prefix(depth);
  printf("%s", tea_node_type_name(node->type));

  const tea_tok_t *token = node->tok;
  if (token && node->type != TEA_N_STR) {
    if (node->type == TEA_N_INT) {
      printf(": %d", *(int *)token->buf);
    } else if (node->type == TEA_N_FLOAT) {
      printf(": %f", *(float *)token->buf);
    } else {
      printf(": %.*s", token->size, token->buf);
    }
  }

  if (token && token->line > 0 && token->col > 0) {
    printf(" (line %d, col %d)", token->line, token->col);
  }

  printf("\n");

  if (node->type == TEA_N_BINOP || node->type == TEA_N_ASSIGN) {
    if (node->binop.lhs) {
      tea_node_print_tree_recursive(node->binop.lhs, depth + 1);
    }
    if (node->binop.rhs) {
      tea_node_print_tree_recursive(node->binop.rhs, depth + 1);
    }
  } else if (node->type == TEA_N_FIELD_ACC) {
    if (node->field_acc.obj) {
      tea_node_print_tree_recursive(node->field_acc.obj, depth + 1);
    }
    if (node->field_acc.field) {
      tea_node_print_tree_recursive(node->field_acc.field, depth + 1);
    }
  } else {
    rtl_list_entry_t *entry;
    rtl_list_for_each(entry, &node->children)
    {
      tea_node_t *child = rtl_list_record(entry, tea_node_t, link);
      tea_node_print_tree_recursive(child, depth + 1);
    }
  }
}

#endif

void tea_node_print(tea_node_t *node, const int depth)
{
#ifdef TEA_DEBUG_BUILD
  tea_node_print_tree_recursive(node, depth);
#endif
}

void tea_node_set_binop(tea_node_t *parent, tea_node_t *lhs, tea_node_t *rhs)
{
  parent->binop.lhs = lhs;
  parent->binop.rhs = rhs;
}

void tea_node_set_field_acc(tea_node_t *parent, tea_node_t *obj,
                            tea_node_t *field)
{
  parent->field_acc.obj = obj;
  parent->field_acc.field = field;
}
