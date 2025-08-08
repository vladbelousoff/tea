#include "tea_struct.h"

#include "tea_expr.h"
#include "tea_fn.h"

#include <stdlib.h>
#include <string.h>

#include <rtl_log.h>
#include <rtl_memory.h>

bool tea_interp_struct_decl(tea_ctx_t *ctx, const tea_node_t *node)
{
  tea_struct_decl_t *struct_declaration =
    rtl_malloc(sizeof(*struct_declaration));
  if (!struct_declaration) {
    return false;
  }

  struct_declaration->node = node;
  struct_declaration->field_cnt = rtl_list_length(&node->children);
  rtl_list_init(&struct_declaration->fns);
  rtl_list_add_tail(&ctx->structs, &struct_declaration->link);

  tea_tok_t *name = node->tok;
  rtl_log_dbg("Declare struct '%s'", name ? name->buf : "");

  return true;
}

tea_struct_decl_t *tea_find_struct_decl(const tea_ctx_t *ctx, const char *name)
{
  rtl_list_entry_t *struct_entry;
  rtl_list_for_each(struct_entry, &ctx->structs)
  {
    tea_struct_decl_t *decl =
      rtl_list_record(struct_entry, tea_struct_decl_t, link);
    if (!decl) {
      rtl_log_err(
        "Internal error: Struct declaration list contains null entry");
      continue;
    }
    if (!decl->node) {
      rtl_log_err("Internal error: Struct declaration has null AST node");
      continue;
    }
    const tea_tok_t *decl_token = decl->node->tok;
    if (decl_token) {
      if (!strcmp(decl_token->buf, name)) {
        return decl;
      }
    }
  }

  return NULL;
}

bool tea_interp_impl_blk(const tea_ctx_t *ctx, const tea_node_t *node)
{
  const tea_tok_t *block_name = node->tok;
  if (!block_name) {
    rtl_log_err("Runtime error: Implementation block must have a struct name");
    return false;
  }

  tea_struct_decl_t *struct_declaration =
    tea_find_struct_decl(ctx, block_name->buf);
  if (!struct_declaration) {
    rtl_log_err(
      "Runtime error: Cannot implement methods for undeclared struct '%s'",
      block_name->buf);
    return false;
  }

  rtl_list_entry_t *entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t *child = rtl_list_record(entry, tea_node_t, link);
    if (child->type != TEA_N_IMPL_ITEM) {
      rtl_log_err(
        "Runtime error: Implementation block contains invalid item - only method implementations "
        "are allowed");
      return false;
    }
    const rtl_list_entry_t *function_entry = rtl_list_first(&child->children);
    if (!function_entry) {
      rtl_log_err(
        "Runtime error: Implementation item is empty - method definition required");
      return false;
    }
    const tea_node_t *function_node =
      rtl_list_record(function_entry, tea_node_t, link);
    if (!function_node || function_node->type != TEA_N_FN) {
      rtl_log_err(
        "Runtime error: Implementation item must contain a method definition");
      return false;
    }

    const bool result = tea_decl_fn(function_node, &struct_declaration->fns);
    if (!result) {
      return false;
    }
  }

  return true;
}

tea_val_t tea_eval_new(tea_ctx_t *ctx, tea_scope_t *scp, const tea_node_t *node)
{
  const tea_tok_t *struct_name = node->tok;
  if (!struct_name) {
    rtl_log_err("Runtime error: Struct instantiation missing type name");
    return tea_val_undef();
  }

  const tea_struct_decl_t *struct_declr =
    tea_find_struct_decl(ctx, struct_name->buf);

  tea_inst_t *object = rtl_malloc(sizeof(tea_inst_t) +
                                  struct_declr->field_cnt * sizeof(tea_val_t));
  if (!object) {
    rtl_log_err(
      "Memory error: Failed to allocate memory for struct '%s' instance at line %d, column %d",
      struct_name->buf, struct_name->line, struct_name->col);
    return tea_val_undef();
  }

  rtl_list_entry_t *field_entry = rtl_list_first(&struct_declr->node->children);
  rtl_list_entry_t *declr_entry;

  unsigned long field_index = 0;

  rtl_list_for_each(declr_entry, &node->children)
  {
    const tea_node_t *declr_node =
      rtl_list_record(declr_entry, tea_node_t, link);
    if (!declr_node) {
      rtl_log_err(
        "Runtime error: Invalid field declaration in struct instantiation");
      rtl_free(object);
      return tea_val_undef();
    }

    const tea_node_t *field_node =
      rtl_list_record(field_entry, tea_node_t, link);

    const tea_node_t *value_node = NULL;
    rtl_list_entry_t *first_child_entry = rtl_list_first(&declr_node->children);
    if (first_child_entry) {
      value_node = rtl_list_record(first_child_entry, tea_node_t, link);
    } else {
      // Use the name of the field as the value,
      // so for example 'new Vec3 {x: x, y: y, z: z}' could be
      // shortened to 'new Vec3 {x, y, z}'
      value_node = declr_node;
    }

    if (!value_node) {
      rtl_log_err("Runtime error: Invalid field value in struct instantiation");
      rtl_free(object);
      return tea_val_undef();
    }

    const tea_tok_t *declr = declr_node->tok;
    const tea_tok_t *field = field_node->tok;

    if (!declr || !field) {
      rtl_log_err(
        "Runtime error: Missing field name or value tokens during struct instantiation");
      rtl_free(object);
      return tea_val_undef();
    }

    if (declr->size != field->size ||
        strncmp(declr->buf, field->buf, field->size) != 0) {
      rtl_log_err(
        "Runtime error: Struct fields must be initialized in declaration order: field '%s' (line: "
        "%d) does not match expected field '%s' (line: %d)",
        declr->buf, declr->line, field->buf, field->line);
      rtl_free(object);
      return tea_val_undef();
    }

    tea_val_t value_expr = tea_eval_expr(ctx, scp, value_node);
    if (value_expr.type == TEA_V_UNDEF) {
      rtl_free(object);
      return tea_val_undef();
    }
    memcpy(&object->buf[field_index++ * sizeof(tea_val_t)], &value_expr,
           sizeof(tea_val_t));

    field_entry = rtl_list_next(field_entry, &struct_declr->node->children);
  }

  object->type = struct_name->buf;

  const tea_val_t result = { .type = TEA_V_INST, .obj = object };
  return result;
}

tea_val_t *tea_get_field_ptr(const tea_ctx_t *ctx, const tea_scope_t *scp,
                             const tea_node_t *node)
{
  // Extract field information
  tea_node_t *field_node = node->field_acc.field;
  if (!field_node) {
    rtl_log_err(
      "Internal error: Field access expression missing field component in AST");
    return NULL;
  }

  const tea_tok_t *field_name = field_node->tok;
  if (!field_name) {
    rtl_log_err("Internal error: Field AST node missing token information");
    return NULL;
  }

  // Extract object information
  tea_node_t *object_node = node->field_acc.obj;
  if (!object_node) {
    rtl_log_err(
      "Internal error: Field access expression missing object component in AST");
    return NULL;
  }

  const tea_tok_t *object_name = object_node->tok;
  if (!object_name) {
    rtl_log_err("Internal error: Object AST node missing token information");
    return NULL;
  }

  // Get the variable containing the object
  const tea_var_t *variable = tea_scope_find(scp, object_name->buf);
  if (!variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' not found in current scope when accessing field (line %d, col "
      "%d)",
      object_name->buf, object_name->line, object_name->col);
    return NULL;
  }

  if (variable->val.type != TEA_V_INST) {
    rtl_log_err(
      "Runtime error: Variable '%s' has type '%s' but field access requires an object instance "
      "(line %d, col %d)",
      object_name->buf, tea_val_type_str(variable->val.type), object_name->line,
      object_name->col);
    return NULL;
  }
  const tea_inst_t *object = variable->val.obj;

  // Find the struct declaration for this object type
  const tea_struct_decl_t *struct_declr =
    tea_find_struct_decl(ctx, object->type);
  if (!struct_declr) {
    rtl_log_err(
      "Runtime error: Cannot find struct declaration for type '%s' when accessing field '%s' (line "
      "%d, col %d)",
      object->type, field_name->buf, field_name->line, field_name->col);
    return NULL;
  }

  // Find the field by name and return its value
  unsigned long field_index = 0;
  rtl_list_entry_t *field_entry;

  rtl_list_for_each_indexed(field_index, field_entry,
                            &struct_declr->node->children)
  {
    if (field_index >= struct_declr->field_cnt) {
      rtl_log_err(
        "Internal error: Field index exceeds struct field count during field access");
      return NULL;
    }

    const tea_node_t *field_decl_node =
      rtl_list_record(field_entry, tea_node_t, link);
    if (!field_decl_node || !field_decl_node->tok) {
      rtl_log_err(
        "Internal error: Invalid field declaration node in struct definition");
      return NULL;
    }

    if (!strcmp(field_decl_node->tok->buf, field_name->buf)) {
      return (tea_val_t *)&object->buf[field_index * sizeof(tea_val_t)];
    }
  }

  return NULL;
}

tea_val_t tea_eval_field_acc(const tea_ctx_t *ctx, const tea_scope_t *scp,
                             const tea_node_t *node)
{
  const tea_val_t *result = tea_get_field_ptr(ctx, scp, node);
  if (result) {
    return *result;
  }

  return tea_val_undef();
}
