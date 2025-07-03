#pragma once

#include "rtl_list.h"

typedef struct
{
  const char *keyword;
  int token_type;
} tea_keyword_entry_t;

typedef struct
{
  rtl_list_entry_t link;
  int type;
  int line;
  int column;
  int position;
  int buffer_size;
  char buffer[0];
} tea_token_t;

int tea_get_ident_token_type(const char *ident, int length);
