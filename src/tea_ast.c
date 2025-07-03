#include "tea_ast.h"

#include "rtl_memory.h"

#include <rtl_log.h>
#include <string.h>

tea_ast_node_t *__tea_ast_node_create(
  const char *file, const unsigned long line, const tea_ast_node_type_t type, tea_token_t *token)
{
  tea_ast_node_t *node = __rtl_malloc(file, line, sizeof(*node));
  if (!node) {
    rtl_log_err("Failed to allocate memory for AST node!\n");
    return NULL;
  }

  node->type = type;
  node->token = token;
  rtl_list_init(&node->children);

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
  rtl_list_entry_t *entry;
  rtl_list_entry_t *safe;

  rtl_list_for_each_safe(entry, safe, &node->children)
  {
    tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
    rtl_list_remove(entry);
    tea_ast_node_free(child);
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
    case TEA_AST_NODE_PARAM:
      return "PARAM";
    case TEA_AST_NODE_ATTR:
      return "ATTR";
    default:
      return "UNKNOWN";
  }
}

void tea_ast_node_print(tea_ast_node_t *node, const int depth)
{
  if (!node) {
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("  ");
  }

  printf("%s", get_node_type_name(node->type));

  const tea_token_t *token = node->token;
  if (token) {
    printf(": %.*s", token->buffer_size, token->buffer);
  }

  if (token && token->line > 0 && token->column > 0) {
    printf(" (line %d, col %d)", token->line, token->column);
  }

  printf("\n");

  rtl_list_entry_t *entry;
  rtl_list_for_each(entry, &node->children)
  {
    tea_ast_node_t *child = rtl_list_record(entry, tea_ast_node_t, link);
    tea_ast_node_print(child, depth + 1);
  }
}
