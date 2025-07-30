#pragma once

#include "tea_scope.h"
#include "tea_value.h"

// Context type is defined in tea_scope.h

// Main expression evaluation function
tea_value_t tea_interpret_evaluate_expression(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);

// Specific evaluation functions
tea_value_t tea_interpret_evaluate_integer_number(tea_token_t* token);
tea_value_t tea_interpret_evaluate_float_number(tea_token_t* token);
tea_value_t tea_interpret_evaluate_binop(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);
tea_value_t tea_interpret_evaluate_unary(
  tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);
tea_value_t tea_interpret_evaluate_ident(const tea_scope_t* scope, const tea_ast_node_t* node);
tea_value_t tea_interpret_evaluate_string(const tea_ast_node_t* node);
