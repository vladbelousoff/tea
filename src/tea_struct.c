#include "tea_struct.h"

#include "tea_expression.h"
#include "tea_function.h"

#include <stdlib.h>
#include <string.h>

#include <rtl.h>
#include <rtl_log.h>
#include <rtl_memory.h>

bool tea_interpret_struct_declaration(tea_context_t* context, const tea_ast_node_t* node)
{
  tea_struct_declaration_t* struct_declaration = rtl_malloc(sizeof(*struct_declaration));
  if (!struct_declaration) {
    return false;
  }

  struct_declaration->node = node;
  struct_declaration->field_count = rtl_list_length(&node->children);
  rtl_list_init(&struct_declaration->functions);
  rtl_list_add_tail(&context->struct_declarations, &struct_declaration->link);

  tea_token_t* name = node->token;
  rtl_log_dbg("Declare struct '%s'", name ? name->buffer : "");

  return true;
}

tea_struct_declaration_t* tea_find_struct_declaration(
  const tea_context_t* context, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->struct_declarations)
  {
    tea_struct_declaration_t* declaration = rtl_list_record(entry, tea_struct_declaration_t, link);
    if (!declaration) {
      rtl_log_err("Internal error: Struct declaration list contains null entry");
      continue;
    }
    if (!declaration->node) {
      rtl_log_err("Internal error: Struct declaration has null AST node");
      continue;
    }
    const tea_token_t* declaration_token = declaration->node->token;
    if (declaration_token) {
      if (!strcmp(declaration_token->buffer, name)) {
        return declaration;
      }
    }
  }

  return NULL;
}

bool tea_interpret_impl_block(const tea_context_t* context, const tea_ast_node_t* node)
{
  const tea_token_t* block_name = node->token;
  if (!block_name) {
    rtl_log_err("Runtime error: Implementation block must have a struct name");
    return false;
  }

  tea_struct_declaration_t* struct_declaration =
    tea_find_struct_declaration(context, block_name->buffer);
  if (!struct_declaration) {
    rtl_log_err(
      "Runtime error: Cannot implement methods for undeclared struct '%s'", block_name->buffer);
    return false;
  }

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_ast_node_t* child = rtl_list_record(entry, tea_ast_node_t, link);
    if (child->type != TEA_AST_NODE_IMPL_ITEM) {
      rtl_log_err(
        "Runtime error: Implementation block contains invalid item - only method implementations "
        "are allowed");
      return false;
    }
    const rtl_list_entry_t* function_entry = rtl_list_first(&child->children);
    if (!function_entry) {
      rtl_log_err("Runtime error: Implementation item is empty - method definition required");
      return false;
    }
    const tea_ast_node_t* function_node = rtl_list_record(function_entry, tea_ast_node_t, link);
    if (!function_node || function_node->type != TEA_AST_NODE_FUNCTION) {
      rtl_log_err("Runtime error: Implementation item must contain a method definition");
      return false;
    }

    const bool result = tea_declare_function(function_node, &struct_declaration->functions);
    if (!result) {
      return false;
    }
  }

  return true;
}

tea_value_t tea_interpret_evaluate_new(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_token_t* struct_name = node->token;
  if (!struct_name) {
    rtl_log_err("Runtime error: Struct instantiation missing type name");
    return tea_value_invalid();
  }

  const tea_struct_declaration_t* struct_declr =
    tea_find_struct_declaration(context, struct_name->buffer);

  tea_instance_t* object =
    rtl_malloc(sizeof(tea_instance_t) + struct_declr->field_count * sizeof(tea_value_t));
  if (!object) {
    rtl_log_err(
      "Memory error: Failed to allocate memory for struct '%s' instance at line %d, column %d",
      struct_name->buffer, struct_name->line, struct_name->column);
    return tea_value_invalid();
  }

  rtl_list_entry_t* field_entry = rtl_list_first(&struct_declr->node->children);
  rtl_list_entry_t* declr_entry;

  unsigned long field_index = 0;

  rtl_list_for_each(declr_entry, &node->children)
  {
    const tea_ast_node_t* declr_node = rtl_list_record(declr_entry, tea_ast_node_t, link);
    if (!declr_node) {
      rtl_log_err("Runtime error: Invalid field declaration in struct instantiation");
      rtl_free(object);
      return tea_value_invalid();
    }

    const tea_ast_node_t* field_node = rtl_list_record(field_entry, tea_ast_node_t, link);

    const tea_ast_node_t* value_node = NULL;
    rtl_list_entry_t* first_child_entry = rtl_list_first(&declr_node->children);
    if (first_child_entry) {
      value_node = rtl_list_record(first_child_entry, tea_ast_node_t, link);
    } else {
      // Use the name of the field as the value,
      // so for example 'new Vec3 {x: x, y: y, z: z}' could be
      // shortened to 'new Vec3 {x, y, z}'
      value_node = declr_node;
    }

    if (!value_node) {
      rtl_log_err("Runtime error: Invalid field value in struct instantiation");
      rtl_free(object);
      return tea_value_invalid();
    }

    const tea_token_t* declr = declr_node->token;
    const tea_token_t* field = field_node->token;

    if (!declr || !field) {
      rtl_log_err("Runtime error: Missing field name or value tokens during struct instantiation");
      rtl_free(object);
      return tea_value_invalid();
    }

    if (declr->buffer_size != field->buffer_size ||
        strncmp(declr->buffer, field->buffer, field->buffer_size) != 0) {
      rtl_log_err(
        "Runtime error: Struct fields must be initialized in declaration order: field '%s' (line: "
        "%d) does not match expected field '%s' (line: %d)",
        declr->buffer, declr->line, field->buffer, field->line);
      rtl_free(object);
      return tea_value_invalid();
    }

    tea_value_t value_expr = tea_interpret_evaluate_expression(context, scope, value_node);
    if (value_expr.type == TEA_VALUE_INVALID) {
      rtl_free(object);
      return tea_value_invalid();
    }
    memcpy(&object->buffer[field_index++ * sizeof(tea_value_t)], &value_expr, sizeof(tea_value_t));

    field_entry = rtl_list_next(field_entry, &struct_declr->node->children);
  }

  object->type = struct_name->buffer;

  const tea_value_t result = { .type = TEA_VALUE_INSTANCE, .object = object };
  return result;
}

tea_value_t* tea_get_field_pointer(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node)
{
  // Extract field information
  tea_ast_node_t* field_node = node->field_access.field;
  if (!field_node) {
    rtl_log_err("Internal error: Field access expression missing field component in AST");
    return NULL;
  }

  const tea_token_t* field_name = field_node->token;
  if (!field_name) {
    rtl_log_err("Internal error: Field AST node missing token information");
    return NULL;
  }

  // Extract object information
  tea_ast_node_t* object_node = node->field_access.object;
  if (!object_node) {
    rtl_log_err("Internal error: Field access expression missing object component in AST");
    return NULL;
  }

  const tea_token_t* object_name = object_node->token;
  if (!object_name) {
    rtl_log_err("Internal error: Object AST node missing token information");
    return NULL;
  }

  // Get the variable containing the object
  const tea_variable_t* variable = tea_scope_find_variable(scope, object_name->buffer);
  if (!variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' not found in current scope when accessing field (line %d, col "
      "%d)",
      object_name->buffer, object_name->line, object_name->column);
    return NULL;
  }

  if (variable->value.type != TEA_VALUE_INSTANCE) {
    rtl_log_err(
      "Runtime error: Variable '%s' has type '%s' but field access requires an object instance "
      "(line %d, col %d)",
      object_name->buffer, tea_value_get_type_string(variable->value.type), object_name->line,
      object_name->column);
    return NULL;
  }
  const tea_instance_t* object = variable->value.object;

  // Find the struct declaration for this object type
  const tea_struct_declaration_t* struct_declr = tea_find_struct_declaration(context, object->type);
  if (!struct_declr) {
    rtl_log_err(
      "Runtime error: Cannot find struct declaration for type '%s' when accessing field '%s' (line "
      "%d, col %d)",
      object->type, field_name->buffer, field_name->line, field_name->column);
    return NULL;
  }

  // Find the field by name and return its value
  unsigned long field_index = 0;
  rtl_list_entry_t* field_entry;

  rtl_list_for_each_indexed(field_index, field_entry, &struct_declr->node->children)
  {
    if (field_index >= struct_declr->field_count) {
      rtl_log_err("Internal error: Field index exceeds struct field count during field access");
      return NULL;
    }

    const tea_ast_node_t* field_decl_node = rtl_list_record(field_entry, tea_ast_node_t, link);
    if (!field_decl_node || !field_decl_node->token) {
      rtl_log_err("Internal error: Invalid field declaration node in struct definition");
      return NULL;
    }

    if (!strcmp(field_decl_node->token->buffer, field_name->buffer)) {
      return (tea_value_t*)&object->buffer[field_index * sizeof(tea_value_t)];
    }
  }

  return NULL;
}

tea_value_t tea_interpret_field_access(
  const tea_context_t* context, const tea_scope_t* scope, const tea_ast_node_t* node)
{
  const tea_value_t* result = tea_get_field_pointer(context, scope, node);
  if (result) {
    return *result;
  }

  return tea_value_invalid();
}
