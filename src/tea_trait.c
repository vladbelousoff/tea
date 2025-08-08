#include "tea_trait.h"

#include "tea_fn.h"
#include "tea_struct.h"

#include <stdlib.h>
#include <string.h>

#include <rtl_log.h>
#include <rtl_memory.h>

bool tea_interpret_trait_declaration(tea_ctx_t* context, const tea_node_t* node)
{
  tea_trait_decl_t* trait_declaration = rtl_malloc(sizeof(*trait_declaration));
  if (!trait_declaration) {
    return false;
  }

  trait_declaration->node = node;
  trait_declaration->name = node->tok ? node->tok->buf : NULL;
  rtl_list_init(&trait_declaration->methods);
  rtl_list_add_tail(&context->traits, &trait_declaration->link);

  tea_tok_t* name = node->tok;
  rtl_log_dbg("Declare trait '%s'", name ? name->buf : "");

  // Process trait methods (just declarations, no implementations)
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    if (child->type == TEA_N_TRAIT_METHOD) {
      const rtl_list_entry_t* function_entry = rtl_list_first(&child->children);
      if (function_entry) {
        const tea_node_t* function_node = rtl_list_record(function_entry, tea_node_t, link);
        if (function_node && function_node->type == TEA_N_FN) {
          const bool result = tea_declare_function(function_node, &trait_declaration->methods);
          if (!result) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

tea_trait_decl_t* tea_find_trait_declaration(const tea_ctx_t* context, const char* name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->traits)
  {
    tea_trait_decl_t* declaration = rtl_list_record(entry, tea_trait_decl_t, link);
    if (!declaration) {
      rtl_log_err("Internal error: Trait declaration list contains null entry");
      continue;
    }
    if (declaration->name && !strcmp(declaration->name, name)) {
      return declaration;
    }
  }

  return NULL;
}

bool tea_interpret_trait_implementation(tea_ctx_t* context, const tea_node_t* node)
{
  const tea_tok_t* trait_name_token = node->tok;
  if (!trait_name_token) {
    rtl_log_err("Runtime error: Trait implementation must have a trait name");
    return false;
  }

  // Get the struct name from the first child
  const rtl_list_entry_t* first_child_entry = rtl_list_first(&node->children);
  if (!first_child_entry) {
    rtl_log_err("Runtime error: Trait implementation must specify a struct name");
    return false;
  }

  const tea_node_t* struct_name_node = rtl_list_record(first_child_entry, tea_node_t, link);
  if (!struct_name_node || !struct_name_node->tok) {
    rtl_log_err("Runtime error: Invalid struct name in trait implementation");
    return false;
  }

  const tea_tok_t* struct_name_token = struct_name_node->tok;

  // Verify the trait exists
  tea_trait_decl_t* trait_declaration = tea_find_trait_declaration(context, trait_name_token->buf);
  if (!trait_declaration) {
    rtl_log_err("Runtime error: Cannot implement undefined trait '%s'", trait_name_token->buf);
    return false;
  }

  // Verify the struct exists
  tea_struct_decl_t* struct_declaration = tea_find_struct_decl(context, struct_name_token->buf);
  if (!struct_declaration) {
    rtl_log_err("Runtime error: Cannot implement trait '%s' for undefined struct '%s'",
      trait_name_token->buf, struct_name_token->buf);
    return false;
  }

  tea_trait_impl_t* trait_impl = rtl_malloc(sizeof(*trait_impl));
  if (!trait_impl) {
    return false;
  }

  trait_impl->node = node;
  trait_impl->trait_name = trait_name_token->buf;
  trait_impl->struct_name = struct_name_token->buf;
  rtl_list_init(&trait_impl->methods);
  rtl_list_add_tail(&context->impls, &trait_impl->link);

  rtl_log_dbg(
    "Implement trait '%s' for struct '%s'", trait_impl->trait_name, trait_impl->struct_name);

  // Process trait implementation methods
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    // Skip the first child (struct name node)
    if (child == struct_name_node) {
      continue;
    }

    if (child->type == TEA_N_TRAIT_IMPL_ITEM) {
      const rtl_list_entry_t* function_entry = rtl_list_first(&child->children);
      if (function_entry) {
        const tea_node_t* function_node = rtl_list_record(function_entry, tea_node_t, link);
        if (function_node && function_node->type == TEA_N_FN) {
          const bool result = tea_declare_function(function_node, &trait_impl->methods);
          if (!result) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

tea_trait_impl_t* tea_find_trait_implementation(
  const tea_ctx_t* context, const char* trait_name, const char* struct_name)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->impls)
  {
    tea_trait_impl_t* impl = rtl_list_record(entry, tea_trait_impl_t, link);
    if (!impl) {
      rtl_log_err("Internal error: Trait implementation list contains null entry");
      continue;
    }
    if (impl->trait_name && impl->struct_name && !strcmp(impl->trait_name, trait_name) &&
        !strcmp(impl->struct_name, struct_name)) {
      return impl;
    }
  }

  return NULL;
}

const tea_fn_t* tea_resolve_trait_method(
  const tea_ctx_t* context, const char* struct_name, const char* method_name)
{
  // Look through all trait implementations for this struct
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &context->impls)
  {
    tea_trait_impl_t* impl = rtl_list_record(entry, tea_trait_impl_t, link);
    if (!impl || !impl->struct_name) {
      continue;
    }

    if (!strcmp(impl->struct_name, struct_name)) {
      // Found an implementation for this struct, check if it has the method
      const tea_fn_t* function = tea_ctx_find_fn(&impl->methods, method_name);
      if (function) {
        return function;
      }
    }
  }

  return NULL;
}
