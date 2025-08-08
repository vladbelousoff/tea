#pragma once

#include "tea_ast.h"
#include "tea_lexer.h"

tea_node_t *tea_parse_file(tea_lexer_t *lex, const char *fname);
