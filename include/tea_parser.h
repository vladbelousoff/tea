#pragma once

#include "tea_ast.h"

tea_ast_node_t *tea_parse_string(const char *input);
tea_ast_node_t *tea_parse_file(const char *filename);
