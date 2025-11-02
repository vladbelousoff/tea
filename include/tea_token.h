#pragma once

#include "tea_list.h"

typedef struct {
  const char *kw;
  int type;
} tea_kw_entry_t;

typedef struct {
  tea_list_entry_t link;
  int type;
  int line;
  int col;
  int pos;
  int size;
  char buf[0];
} tea_tok_t;

int tea_get_ident_type(const char *ident, int length);
const char *tea_tok_name(int token_type);
