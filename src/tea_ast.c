#include "tea_ast.h"

#include "rtl_memory.h"

#include <rtl_log.h>
#include <string.h>

tea_ast_node_t *_tea_ast_node_create(
  const char *file, const unsigned long line, const tea_ast_node_type_t type, tea_token_t *token)
{
  tea_ast_node_t *node = _rtl_malloc(file, line, sizeof(*node));
  if (!node) {
    rtl_log_err("Failed to allocate memory for AST node!");
    return NULL;
  }

  node->type = type;
  node->token = token;

  // Initialize union based on node type
  if (type == TEA_AST_NODE_BINOP) {
    node->binop.lhs = NULL;
    node->binop.rhs = NULL;
  } else {
    rtl_list_init(&node->children);
  }

  return node;
}

void tea_ast_node_add_child(tea_ast_node_t *parent, tea_ast_node_t *child)
{
  if (!parent || !child) {
    return;
  }

  rtl_list_add_tail(&parent->children, &child->link);
}

void tea_ast_node_add_children(tea_ast_node_t *parent, const rtl_list_entry_t *children)
{
  rtl_list_entry_t *entry;
  rtl_list_entry_t *safe;
  rtl_list_for_each_safe(entry, safe, children)
  {
    tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
    rtl_list_remove(entry);
    tea_ast_node_add_child(parent, child);
  }
}

void tea_ast_node_free(tea_ast_node_t *node)
{
  if (!node) {
    return;
  }

  if (node->type == TEA_AST_NODE_BINOP) {
    if (node->binop.lhs) {
      tea_ast_node_free(node->binop.lhs);
    }
    if (node->binop.rhs) {
      tea_ast_node_free(node->binop.rhs);
    }
  } else {
    rtl_list_entry_t *entry;
    rtl_list_entry_t *safe;

    rtl_list_for_each_safe(entry, safe, &node->children)
    {
      tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
      rtl_list_remove(entry);
      tea_ast_node_free(child);
    }
  }

  rtl_free(node);
}

static const char *get_node_type_name(const tea_ast_node_type_t type)
{
  switch (type) {
    case TEA_AST_NODE_PROGRAM:
      return "PROGRAM";
    case TEA_AST_NODE_FUNCTION:
      return "FUNCTION";
    case TEA_AST_NODE_NATIVE_FUNCTION:
      return "NATIVE_FUNCTION";
    case TEA_AST_NODE_PARAM:
      return "PARAM";
    case TEA_AST_NODE_ATTR:
      return "ATTR";
    case TEA_AST_NODE_STMT:
      return "STMT";
    case TEA_AST_NODE_LET:
      return "LET";
    case TEA_AST_NODE_MUT:
      return "MUT";
    case TEA_AST_NODE_ASSIGN:
      return "ASSIGN";
    case TEA_AST_NODE_BINOP:
      return "BINOP";
    case TEA_AST_NODE_UNARY:
      return "UNARY";
    case TEA_AST_NODE_IDENT:
      return "IDENT";
    case TEA_AST_NODE_NUMBER:
      return "NUMBER";
    case TEA_AST_NODE_CALL:
      return "CALL";
    case TEA_AST_NODE_RETURN:
      return "RETURN";
    case TEA_AST_NODE_IF:
      return "IF";
    case TEA_AST_NODE_THEN:
      return "THEN";
    case TEA_AST_NODE_ELSE:
      return "ELSE";
    case TEA_AST_NODE_WHILE:
      return "WHILE";
    case TEA_AST_NODE_STRUCT:
      return "STRUCT";
    case TEA_AST_NODE_STRUCT_FIELD:
      return "STRUCT_FIELD";
    case TEA_AST_NODE_STRUCT_INSTANCE:
      return "STRUCT_INSTANCE";
    case TEA_AST_NODE_STRUCT_FIELD_INIT:
      return "STRUCT_FIELD_INIT";
    case TEA_AST_NODE_STRING:
      return "STRING";
    case TEA_AST_NODE_FIELD_ACCESS:
      return "FIELD_ACCESS";
    case TEA_AST_NODE_IMPL_ITEM:
      return "IMPL_ITEM";
    case TEA_AST_NODE_IMPL_BLOCK:
      return "IMPL_BLOCK";
    default:
      return "UNKNOWN";
  }
}

static void print_tree_prefix(const int depth, const bool *is_last_stack)
{
  for (int i = 0; i < depth; i++) {
    if (i == depth - 1) {
      printf(is_last_stack[i] ? "`- " : "|- ");
    } else {
      printf(is_last_stack[i] ? "   " : "|  ");
    }
  }
}

static void tea_ast_node_print_tree_recursive(
  tea_ast_node_t *node, const int depth, bool *is_last_stack)
{
  if (!node) {
    return;
  }

  print_tree_prefix(depth, is_last_stack);
  printf("%s", get_node_type_name(node->type));

  const tea_token_t *token = node->token;
  if (token) {
    if (node->type == TEA_AST_NODE_STRING) {
      printf(": '%.*s'", token->buffer_size, token->buffer);
    } else {
      printf(": %.*s", token->buffer_size, token->buffer);
    }
  }

  if (token && token->line > 0 && token->column > 0) {
    printf(" (line %d, col %d)", token->line, token->column);
  }

  printf("\n");

  if (node->type == TEA_AST_NODE_BINOP) {
    if (node->binop.lhs) {
      is_last_stack[depth] = !node->binop.rhs;
      tea_ast_node_print_tree_recursive(node->binop.lhs, depth + 1, is_last_stack);
    }
    if (node->binop.rhs) {
      is_last_stack[depth] = true;
      tea_ast_node_print_tree_recursive(node->binop.rhs, depth + 1, is_last_stack);
    }
  } else {
    rtl_list_entry_t *entry;
    int child_count = 0;

    // Count children first
    rtl_list_for_each(entry, &node->children)
    {
      child_count++;
    }

    int current_child = 0;
    rtl_list_for_each(entry, &node->children)
    {
      current_child++;
      tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
      is_last_stack[depth] = (current_child == child_count);
      tea_ast_node_print_tree_recursive(child, depth + 1, is_last_stack);
    }
  }
}

void tea_ast_node_print(tea_ast_node_t *node, const int depth)
{
  // Allocate a stack to track which nodes are last children at each depth
  bool is_last_stack[100];  // Assuming max depth of 100
  for (int i = 0; i < 100; i++) {
    is_last_stack[i] = true;
  }
  tea_ast_node_print_tree_recursive(node, depth, is_last_stack);
}

void tea_ast_node_set_binop_children(
  tea_ast_node_t *parent, tea_ast_node_t *lhs, tea_ast_node_t *rhs)
{
  if (!parent || parent->type != TEA_AST_NODE_BINOP) {
    return;
  }

  parent->binop.lhs = lhs;
  parent->binop.rhs = rhs;
}

