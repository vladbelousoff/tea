#pragma once

#include <rtl_list.h>

typedef struct
{
  int pos;
  int line;
  int col;
  rtl_list_entry_t toks;
} tea_lexer_t;

void tea_lexer_init(tea_lexer_t *lex);
void tea_lexer_cleanup(const tea_lexer_t *lex);
void tea_lexer_tokenize(tea_lexer_t *lex, const char *input);
