#pragma once

#include "rtl_list.h"

typedef struct
{
  const char *kw;
  int type;
} tea_kw_entry_t;

typedef struct
{
  rtl_list_entry_t link;
  int type;
  int line;
  int col;
  int pos;
  int size;
  char buf[0];
} tea_tok_t;

int tea_get_ident_type(const char *ident, int len);
const char *tea_tok_name(int type);
