#include "tea_token.h"

#include "tea.h"

#include <string.h>

static const tea_keyword_entry_t tea_keywords[] = { { "fn", TEA_TOKEN_FN }, { NULL, 0 } };

static bool equals(const char *a, const char *b, const int n)
{
  if (n > 0) {
    return !strncmp(a, b, n);
  }

  return !strcmp(a, b);
}

int tea_get_ident_token_type(const char *ident, const int length)
{
  for (int i = 0; tea_keywords[i].keyword; ++i) {
    if (equals(ident, tea_keywords[i].keyword, length)) {
      return tea_keywords[i].token_type;
    }
  }

  return TEA_TOKEN_IDENT;
}
