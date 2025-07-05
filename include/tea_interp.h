#pragma once

#include <stdbool.h>
#include "tea_ast.h"

typedef enum
{
  TEA_VALUE_FLOAT,
  TEA_VALUE_UNDEFINED
} tea_value_type_t;

typedef struct
{
  tea_value_type_t type;
  float float_value;
} tea_value_t;

typedef struct tea_variable
{
  char *name;
  tea_value_t value;
  bool is_mutable;
  struct tea_variable *next;
} tea_variable_t;

typedef struct
{
  tea_variable_t *variables;
} tea_context_t;

void tea_interp_init(tea_context_t *context);
void tea_interp_cleanup(tea_context_t *context);
bool tea_interp_execute(tea_context_t *context, tea_ast_node_t *node);
tea_value_t tea_interp_evaluate(tea_context_t *context, tea_ast_node_t *node);
void tea_value_print(const tea_value_t *value); 