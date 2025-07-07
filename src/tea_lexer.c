#include "tea_lexer.h"

#include "rtl_log.h"
#include "rtl_memory.h"
#include "tea.h"
#include "tea_token.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define EOL        '\n'
#define TAB        '\t'
#define CRR        '\r'
#define EOS        '\0'
#define WHITESPACE ' '

void tea_lexer_init(tea_lexer_t *self)
{
  self->position = 0;
  self->line = 1;
  self->column = 1;
  rtl_list_init(&self->tokens);
}

void tea_lexer_cleanup(const tea_lexer_t *self)
{
  rtl_list_entry_t *entry;
  rtl_list_entry_t *safe;

  rtl_list_for_each_safe(entry, safe, &self->tokens)
  {
    tea_token_t *token = rtl_list_record(entry, tea_token_t, link);
    rtl_list_remove(entry);
    rtl_free(token);
  }
}

static void create_token(
  tea_lexer_t *self, const int token_type, const char *buffer, const int buffer_size)
{
  int token_size = sizeof(tea_token_t);
  if (buffer_size > 0) {
    token_size += buffer_size + 1;
  }

  tea_token_t *token = rtl_malloc(token_size);
  token->type = token_type;
  token->line = self->line;
  token->column = self->column;
  token->position = self->position;
  token->buffer_size = buffer_size;

  if (buffer) {
    memcpy(token->buffer, buffer, buffer_size);
  }

  if (buffer_size > 0) {
    token->buffer[buffer_size] = EOS;
  }

  if (token_type == TEA_TOKEN_IDENT || token_type == TEA_TOKEN_NUMBER) {
    rtl_log_dbg("New token: %.*s", buffer_size, buffer);
  } else {
    rtl_log_dbg("New token: <%s>", tea_get_token_name(token_type));
  }

  rtl_list_add_tail(&self->tokens, &token->link);
}

static void skip_whitespaces(tea_lexer_t *self, const char *input)
{
  while (true) {
    const char c = input[self->position];
    switch (c) {
      case WHITESPACE:
      case CRR:
      case TAB:
        self->column++;
        self->position++;
        break;
      case EOL:
        self->column = 1;
        self->position++;
        self->line++;
        break;
      default:
        return;
    }
  }
}

static bool scan_operator(tea_lexer_t *self, const char *input)
{
  int token_length = 1;
  int token_type;

  switch (input[self->position]) {
    case '@':
      token_type = TEA_TOKEN_AT;
      break;
    case ':':
      token_type = TEA_TOKEN_COLON;
      break;
    case ',':
      token_type = TEA_TOKEN_COMMA;
      break;
    case ';':
      token_type = TEA_TOKEN_SEMICOLON;
      break;
    case '=':
      token_type = TEA_TOKEN_ASSIGN;
      break;
    case '-':
      if (input[self->position + 1] == '>') {
        token_type = TEA_TOKEN_ARROW;
        token_length = 2;
        break;
      }
      token_type = TEA_TOKEN_MINUS;
      break;
    case '+':
      token_type = TEA_TOKEN_PLUS;
      break;
    case '*':
      token_type = TEA_TOKEN_STAR;
      break;
    case '/':
      token_type = TEA_TOKEN_SLASH;
      break;
    case '(':
      token_type = TEA_TOKEN_LPAREN;
      break;
    case ')':
      token_type = TEA_TOKEN_RPAREN;
      break;
    case '{':
      token_type = TEA_TOKEN_LBRACE;
      break;
    case '}':
      token_type = TEA_TOKEN_RBRACE;
      break;
    default:
      return false;
  }

  create_token(self, token_type, &input[self->position], token_length);

  self->column += token_length;
  self->position += token_length;

  return true;
}

static bool scan_number(tea_lexer_t *self, const char *input)
{
  const char first_char = input[self->position];

  if (isdigit(first_char) || (first_char == '.' && isdigit(input[self->position + 1]))) {
    const int start_position = self->position;
    int current_position = start_position;
    bool is_float = false;

    // Scan the number
    while (true) {
      const char c = input[current_position];

      if (c == EOS) {
        break;
      }

      if (isdigit(c)) {
        current_position++;
        continue;
      }

      if (c == '.' && !is_float) {
        if (isdigit(input[current_position + 1])) {
          is_float = true;
          current_position++;
          continue;
        }
      }

      break;
    }

    const int length = current_position - start_position;
    const char *buffer = &input[start_position];

    create_token(self, TEA_TOKEN_NUMBER, buffer, length);

    self->column += length;
    self->position = current_position;

    return true;
  }

  return false;
}

static bool scan_ident(tea_lexer_t *self, const char *input)
{
  char c = input[self->position];
  if (isalpha(c) || c == '_') {
    int token_length = 0;

    while (true) {
      c = input[self->position + token_length];
      if (!isalnum(c) && c != '_') {
        break;
      }

      token_length++;
    }

    const char *token_name = &input[self->position];
    const int token_type = tea_get_ident_token_type(token_name, token_length);
    if (token_type != TEA_TOKEN_IDENT) {
      create_token(self, token_type, NULL, 0);
    } else {
      create_token(self, token_type, token_name, token_length);
    }

    self->position += token_length;
    self->column += token_length;

    return true;
  }

  return false;
}

static void unknown_character(const tea_lexer_t *self, const char *input)
{
  const char c = input[self->position];
  if (c != EOS) {
    rtl_log_err("Unknown character: %c, line: %d, column: %d, position: %d", c, self->line,
      self->column, self->position);
    exit(-1);
  }
}

void tea_lexer_tokenize(tea_lexer_t *self, const char *input)
{
  while (input[self->position] != EOS) {
    skip_whitespaces(self, input);

    if (scan_operator(self, input)) {
      continue;
    }

    if (scan_number(self, input)) {
      continue;
    }

    if (scan_ident(self, input)) {
      continue;
    }

    unknown_character(self, input);
  }
}
