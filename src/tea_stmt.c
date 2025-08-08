#include "tea_stmt.h"
#include "tea_expr.h"
#include "tea_fn.h"
#include "tea_struct.h"
#include "tea_trait.h"

#include <stdlib.h>

#include <rtl.h>
#include <rtl_log.h>

bool tea_exec_stmt(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node,
  tea_ret_ctx_t* return_context, tea_loop_ctx_t* loop_context)
{
  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    if (!tea_exec(context, scope, child, return_context, loop_context)) {
      return false;
    }
  }

  return true;
}

static void tea_extract_type_info(
  const tea_node_t* type_annot, const char** type_name, bool* is_optional)
{
  *type_name = NULL;
  *is_optional = false;

  if (!type_annot || type_annot->type != TEA_N_TYPE_ANNOT) {
    return;
  }

  rtl_list_entry_t* entry = rtl_list_first(&type_annot->children);
  if (!entry) {
    return;
  }

  const tea_node_t* type_spec = rtl_list_record(entry, tea_node_t, link);

  if (type_spec->tok) {
    *type_name = type_spec->tok->buf;
  }

  if (type_spec->type == TEA_N_OPT_TYPE) {
    *is_optional = true;
  }
}

bool tea_interpret_let(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node)
{
  const tea_tok_t* name = node->tok;

  unsigned int flags = 0;
  const tea_node_t* type_annot = NULL;
  const tea_node_t* expr = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    switch (child->type) {
      case TEA_N_MUT:
        flags |= TEA_VAR_MUT;
        break;
      case TEA_N_TYPE_ANNOT:
        type_annot = child;
        break;
      default:
        expr = child;
        break;
    }
  }

  const char* type_name;
  bool is_optional;
  tea_extract_type_info(type_annot, &type_name, &is_optional);

  if (is_optional) {
    flags |= TEA_VAR_OPT;
  }

  return tea_decl_var(context, scope, name->buf, flags, type_name, expr);
}

static bool tea_check_field_mutability(
  const tea_scope_t* scope, const tea_node_t* field_access_node)
{
  tea_node_t* object_node = field_access_node->field_acc.obj;
  if (!object_node || !object_node->tok) {
    rtl_log_err("Internal error: Field access expression missing object component in AST");
    return false;
  }

  const tea_tok_t* object_name = object_node->tok;
  const tea_var_t* variable = tea_scope_find(scope, object_name->buf);
  if (!variable) {
    rtl_log_err(
      "Runtime error: Variable '%s' not found in current scope when checking field mutability, "
      "line: %d, column: %d",
      object_name->buf, object_name->line, object_name->col);
    return false;
  }

  if (!(variable->flags & TEA_VAR_MUT)) {
    rtl_log_err(
      "Runtime error: Cannot modify field of immutable variable '%s' at line %d, column %d",
      object_name->buf, object_name->line, object_name->col);
    return false;
  }

  return true;
}

static bool tea_perform_assignment(tea_val_t* target_value, const tea_val_t new_value,
  const bool is_optional, const char* target_name, const tea_tok_t* error_token)
{
  const bool new_is_null = new_value.type == TEA_V_NULL;
  const bool types_match = new_value.type == target_value->type;
  const bool null_type_match =
    target_value->type == TEA_V_NULL && target_value->null_type == new_value.type;

  if (is_optional && new_is_null) {
    if (new_value.null_type == TEA_V_NULL) {
      target_value->null_type = target_value->type;
      target_value->type = TEA_V_NULL;
    } else {
      *target_value = new_value;
    }
  } else if (types_match || null_type_match) {
    *target_value = new_value;
  } else {
    rtl_log_err(
      "Runtime error: Type mismatch in assignment to '%s%s' at line %d, column %d: cannot "
      "assign %s value to %s target",
      target_name, is_optional ? "?" : "", error_token->line, error_token->col,
      tea_val_type_str(new_value.type), tea_val_type_str(target_value->type));
    return false;
  }

  switch (target_value->type) {
    case TEA_V_I32:
      rtl_log_dbg("New value for %s : %s = %d", target_name, tea_val_type_str(target_value->type),
        target_value->i32);
      break;
    case TEA_V_F32:
      rtl_log_dbg("New value for %s : %s = %f", target_name, tea_val_type_str(target_value->type),
        target_value->f32);
      break;
    default:
      break;
  }

  return true;
}

bool tea_interpret_assign(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node)
{
  const tea_node_t* lhs = node->binop.lhs;
  const tea_node_t* rhs = node->binop.rhs;

  const tea_val_t new_value = tea_eval_expr(context, scope, rhs);
  if (new_value.type == TEA_V_UNDEF) {
    rtl_log_err("Runtime error: Failed to evaluate right-hand side expression in assignment");
    return false;
  }

  if (lhs->type != TEA_N_IDENT) {
    if (!tea_check_field_mutability(scope, lhs)) {
      return false;
    }

    tea_val_t* field_value = tea_get_field_ptr(context, scope, lhs);
    if (!field_value) {
      return false;
    }

    const tea_node_t* field_node = lhs->field_acc.field;
    const tea_tok_t* field_token = field_node ? field_node->tok : NULL;
    const char* field_name = field_token ? field_token->buf : "field";

    return tea_perform_assignment(field_value, new_value, false, field_name, field_token);
  }

  const tea_tok_t* name = lhs->tok;
  tea_var_t* variable = tea_scope_find(scope, name->buf);
  if (!variable) {
    rtl_log_err("Runtime error: Undefined variable '%s' used in assignment at line %d, column %d",
      name->buf, name->line, name->col);
    return false;
  }

  if (!(variable->flags & TEA_VAR_MUT)) {
    rtl_log_err("Runtime error: Cannot modify immutable variable '%s' at line %d, column %d",
      name->buf, name->line, name->col);
    return false;
  }

  const bool is_optional = variable->flags & TEA_VAR_OPT;
  return tea_perform_assignment(&variable->val, new_value, is_optional, name->buf, name);
}

bool tea_exec_if(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node,
  tea_ret_ctx_t* return_context, tea_loop_ctx_t* loop_context)
{
  const tea_node_t* condition = NULL;
  const tea_node_t* then_node = NULL;
  const tea_node_t* else_node = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    switch (child->type) {
      case TEA_N_THEN:
        then_node = child;
        break;
      case TEA_N_ELSE:
        else_node = child;
        break;
      default:
        condition = child;
        break;
    }
  }

  tea_scope_t inner_scope;
  tea_scope_init(&inner_scope, scope);

  const tea_val_t cond_val = tea_eval_expr(context, &inner_scope, condition);
  if (cond_val.type == TEA_V_UNDEF) {
    tea_scope_cleanup(context, &inner_scope);
    return false;
  }

  bool result = true;
  if (cond_val.i32 != 0) {
    result = tea_exec(context, &inner_scope, then_node, return_context, loop_context);
  } else if (else_node) {
    result = tea_exec(context, &inner_scope, else_node, return_context, loop_context);
  }

  tea_scope_cleanup(context, &inner_scope);
  return result;
}

bool tea_exec_while(
  tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node, tea_ret_ctx_t* return_context)
{
  const tea_node_t* while_cond = NULL;
  const tea_node_t* while_body = NULL;

  rtl_list_entry_t* entry;
  rtl_list_for_each(entry, &node->children)
  {
    const tea_node_t* child = rtl_list_record(entry, tea_node_t, link);
    switch (child->type) {
      case TEA_N_WHILE_COND:
        while_cond = rtl_list_record(rtl_list_first(&child->children), tea_node_t, link);
        break;
      case TEA_N_WHILE_BODY:
        while_body = child;
        break;
      default:
        break;
    }
  }

  if (!while_cond) {
    return false;
  }

  tea_loop_ctx_t loop_context;
  loop_context.is_break_set = false;

  while (true) {
    const tea_val_t cond_val = tea_eval_expr(context, scope, while_cond);
    if (cond_val.type == TEA_V_UNDEF) {
      return false;
    }
    if (cond_val.i32 == 0) {
      break;
    }

    // Reset continue flag at the beginning of each iteration
    loop_context.is_cont_set = false;

    tea_scope_t inner_scope;
    tea_scope_init(&inner_scope, scope);
    const bool result = tea_exec(context, &inner_scope, while_body, return_context, &loop_context);
    tea_scope_cleanup(context, &inner_scope);

    if (!result) {
      return false;
    }

    if (return_context && return_context->is_set) {
      break;
    }

    if (loop_context.is_break_set) {
      break;
    }
  }

  return true;
}

bool tea_exec_return(
  tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node, tea_ret_ctx_t* return_context)
{
  rtl_list_entry_t* entry = rtl_list_first(&node->children);
  if (entry) {
    const tea_node_t* expr = rtl_list_record(entry, tea_node_t, link);
    if (expr && return_context) {
      return_context->ret_val = tea_eval_expr(context, scope, expr);
      if (return_context->ret_val.type == TEA_V_UNDEF) {
        return false;
      }
      return_context->is_set = true;
    }
  }

  return true;
}

bool tea_exec_break(tea_loop_ctx_t* loop_context)
{
  if (loop_context) {
    loop_context->is_break_set = true;
    return true;
  }

  rtl_log_err("Runtime error: 'break' statement can only be used inside loops");
  return false;
}

bool tea_exec_continue(tea_loop_ctx_t* loop_context)
{
  if (loop_context) {
    loop_context->is_cont_set = true;
    return true;
  }

  rtl_log_err("Runtime error: 'continue' statement can only be used inside loops");
  return false;
}

bool tea_exec(tea_ctx_t* context, tea_scope_t* scope, const tea_node_t* node,
  tea_ret_ctx_t* return_context, tea_loop_ctx_t* loop_context)
{
  if (return_context && return_context->is_set) {
    return true;
  }

  if (loop_context && loop_context->is_break_set) {
    return true;
  }

  if (loop_context && loop_context->is_cont_set) {
    return true;
  }

  if (!node) {
    rtl_log_err("Node is null");
    return true;
  }

  switch (node->type) {
    case TEA_N_LET:
      return tea_interpret_let(context, scope, node);
    case TEA_N_ASSIGN:
      return tea_interpret_assign(context, scope, node);
    case TEA_N_IF:
      return tea_exec_if(context, scope, node, return_context, loop_context);
    case TEA_N_WHILE:
      return tea_exec_while(context, scope, node, return_context);
    case TEA_N_FN:
      return tea_interpret_function_declaration(context, node);
    case TEA_N_RET:
      return tea_exec_return(context, scope, node, return_context);
    case TEA_N_BREAK:
      return tea_exec_break(loop_context);
    case TEA_N_CONT:
      return tea_exec_continue(loop_context);
    case TEA_N_FN_CALL:
      tea_eval_fn_call(context, scope, node);
      return true;
    case TEA_N_STRUCT:
      return tea_interpret_struct_declaration(context, node);
    case TEA_N_IMPL_BLK:
      return tea_interpret_impl_block(context, node);
    case TEA_N_TRAIT:
      return tea_interpret_trait_declaration(context, node);
    case TEA_N_TRAIT_IMPL:
      return tea_interpret_trait_implementation(context, node);
    case TEA_N_PROG:
    case TEA_N_STMT:
    case TEA_N_FN_ARGS:
    case TEA_N_THEN:
    case TEA_N_ELSE:
    case TEA_N_WHILE_COND:
    case TEA_N_WHILE_BODY:
      return tea_exec_stmt(context, scope, node, return_context, loop_context);
    default: {
      const tea_tok_t* token = node->tok;
      if (token) {
        rtl_log_err(
          "Interpreter error: Unimplemented statement type <%s> in file %s, token: <%s> '%.*s' "
          "(line %d, col %d)",
          tea_node_type_name(node->type), context->fname, tea_tok_name(token->type), token->size,
          token->buf, token->line, token->col);
      } else {
        rtl_log_err("Interpreter error: Unimplemented statement type <%s> in file %s",
          tea_node_type_name(node->type), context->fname);
      }
    } break;
  }

  return false;
}
