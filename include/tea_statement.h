#pragma once

#include "tea_function.h"
#include "tea_scope.h"

typedef struct
{
  unsigned char is_break_set : 1;
  unsigned char is_continue_set : 1;
} tea_loop_context_t;

bool tea_interpret_execute(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node,
  tea_return_context_t* return_context, tea_loop_context_t* loop_context);

bool tea_interpret_execute_stmt(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context,
  tea_loop_context_t* loop_context);

bool tea_interpret_let(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);
bool tea_interpret_assign(tea_context_t* context, tea_scope_t* scope, const tea_ast_node_t* node);
bool tea_interpret_execute_if(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context,
  tea_loop_context_t* loop_context);
bool tea_interpret_execute_while(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context);
bool tea_interpret_execute_return(tea_context_t* context, tea_scope_t* scope,
  const tea_ast_node_t* node, tea_return_context_t* return_context);
bool tea_interpret_execute_break(tea_loop_context_t* loop_context);
bool tea_interpret_execute_continue(tea_loop_context_t* loop_context);
