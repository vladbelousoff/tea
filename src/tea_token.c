#include "tea_token.h"

#include "tea_grammar.h"

#include <string.h>

static const tea_kw_entry_t tea_keywords[] = { { "fn", TEA_TOKEN_FN }, { "let", TEA_TOKEN_LET },
  { "mut", TEA_TOKEN_MUT }, { "if", TEA_TOKEN_IF }, { "else", TEA_TOKEN_ELSE },
  { "while", TEA_TOKEN_WHILE }, { "break", TEA_TOKEN_BREAK }, { "continue", TEA_TOKEN_CONTINUE },
  { "struct", TEA_TOKEN_STRUCT }, { "impl", TEA_TOKEN_IMPL }, { "return", TEA_TOKEN_RETURN },
  { "new", TEA_TOKEN_NEW }, { "null", TEA_TOKEN_NULL }, { "trait", TEA_TOKEN_TRAIT },
  { "for", TEA_TOKEN_FOR }, { NULL, 0 } };

static bool equals(const char *a, const char *b, const int n)
{
  if (n > 0) {
    return !strncmp(a, b, n);
  }

  return !strcmp(a, b);
}

int tea_get_ident_type(const char *ident, const int length)
{
  for (int i = 0; tea_keywords[i].kw; ++i) {
    const size_t keyword_len = strlen(tea_keywords[i].kw);
    if (keyword_len != length) {
      continue;
    }
    if (equals(ident, tea_keywords[i].kw, length)) {
      return tea_keywords[i].type;
    }
  }

  return TEA_TOKEN_IDENT;
}

const char *tea_tok_name(const int token_type)
{
  switch (token_type) {
    case TEA_TOKEN_FN:
      return "FN";
    case TEA_TOKEN_LET:
      return "LET";
    case TEA_TOKEN_MUT:
      return "MUT";
    case TEA_TOKEN_IF:
      return "IF";
    case TEA_TOKEN_ELSE:
      return "ELSE";
    case TEA_TOKEN_WHILE:
      return "WHILE";
    case TEA_TOKEN_BREAK:
      return "BREAK";
    case TEA_TOKEN_CONTINUE:
      return "CONTINUE";
    case TEA_TOKEN_STRUCT:
      return "STRUCT";
    case TEA_TOKEN_IMPL:
      return "IMPL";
    case TEA_TOKEN_RETURN:
      return "RETURN";
    case TEA_TOKEN_NEW:
      return "NEW";
    case TEA_TOKEN_TRAIT:
      return "TRAIT";
    case TEA_TOKEN_FOR:
      return "FOR";
    case TEA_TOKEN_IDENT:
      return "IDENT";
    case TEA_TOKEN_LPAREN:
      return "LPAREN";
    case TEA_TOKEN_RPAREN:
      return "RPAREN";
    case TEA_TOKEN_LBRACE:
      return "LBRACE";
    case TEA_TOKEN_RBRACE:
      return "RBRACE";
    case TEA_TOKEN_AT:
      return "AT";
    case TEA_TOKEN_COLON:
      return "COLON";
    case TEA_TOKEN_COMMA:
      return "COMMA";
    case TEA_TOKEN_SEMICOLON:
      return "SEMICOLON";
    case TEA_TOKEN_ASSIGN:
      return "ASSIGN";
    case TEA_TOKEN_MINUS:
      return "MINUS";
    case TEA_TOKEN_PLUS:
      return "PLUS";
    case TEA_TOKEN_STAR:
      return "STAR";
    case TEA_TOKEN_SLASH:
      return "SLASH";
    case TEA_TOKEN_ARROW:
      return "ARROW";
    case TEA_TOKEN_GT:
      return "GT";
    case TEA_TOKEN_LT:
      return "LT";
    case TEA_TOKEN_EQ:
      return "EQ";
    case TEA_TOKEN_NE:
      return "NE";
    case TEA_TOKEN_GE:
      return "GE";
    case TEA_TOKEN_LE:
      return "LE";
    case TEA_TOKEN_AND:
      return "AND";
    case TEA_TOKEN_OR:
      return "OR";
    case TEA_TOKEN_INTEGER_NUMBER:
      return "INTEGER";
    case TEA_TOKEN_FLOAT_NUMBER:
      return "FLOAT";
    case TEA_TOKEN_STRING:
      return "STRING";
    case TEA_TOKEN_DOT:
      return "DOT";
    case TEA_TOKEN_QUESTION_MARK:
      return "QUESTION_MARK";
    default:
      return NULL;
  }
}
