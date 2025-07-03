#pragma once

#include "tea_ast.h"
#include "tea_lexer.h"

tea_ast_node_t *tea_parse_string(tea_lexer_t *lexer, const char *input);
tea_ast_node_t *tea_parse_file(tea_lexer_t *lexer, const char *filename);
