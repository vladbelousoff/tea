#pragma once

#include <rtl_list.h>

typedef struct
{
  int position;
  int line;
  int column;
  rtl_list_entry_t tokens;
} tea_lexer_t;

void tea_lexer_init(tea_lexer_t *self);
void tea_lexer_cleanup(const tea_lexer_t *self);
void tea_lexer_tokenize(tea_lexer_t *self, const char *input);
