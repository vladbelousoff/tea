#include "tea_lexer.h"

#include "tea_grammar.h"
#include "tea_token.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tea_log.h"
#include "tea_memory.h"

#define EOL        '\n'
#define TAB        '\t'
#define CRR        '\r'
#define EOS        '\0'
#define WHITESPACE ' '

void tea_lexer_init(tea_lexer_t *self)
{
  self->pos = 0;
  self->line = 1;
  self->col = 1;
  tea_list_init(&self->toks);
}

void tea_lexer_cleanup(const tea_lexer_t *self)
{
  tea_list_entry_t *entry;
  tea_list_entry_t *safe;

  tea_list_for_each_safe(entry, safe, &self->toks)
  {
    tea_tok_t *token = tea_list_record(entry, tea_tok_t, link);
    tea_list_remove(entry);
    tea_free(token);
  }
}

static void create_token(tea_lexer_t *self, const int token_type,
                         const char *buffer, const int buffer_size)
{
  int token_size = sizeof(tea_tok_t);
  if (buffer_size > 0) {
    token_size += buffer_size + 1;
  }

  tea_tok_t *token = tea_malloc(token_size);
  token->type = token_type;
  token->line = self->line;
  token->col = self->col;
  token->pos = self->pos;
  token->size = buffer_size;

  if (buffer) {
    memcpy(token->buf, buffer, buffer_size);
  }

  if (buffer_size > 0) {
    token->buf[buffer_size] = EOS;
  }

  if (token_type == TEA_TOKEN_STRING) {
    tea_log_dbg("Token: <STRING> (line: %d, col: %d)", token->line, token->col);
  } else if (token_type == TEA_TOKEN_IDENT) {
    tea_log_dbg("Token: %.*s (line: %d, col: %d)", buffer_size, buffer,
                token->line, token->col);
  } else if (token_type == TEA_TOKEN_INTEGER_NUMBER) {
    tea_log_dbg("Token: %d (line: %d, col: %d)", *(int *)buffer, token->line,
                token->col);
  } else if (token_type == TEA_TOKEN_FLOAT_NUMBER) {
    tea_log_dbg("Token: %f (line: %d, col: %d)", *(float *)buffer, token->line,
                token->col);
  } else {
    tea_log_dbg("Token: <%s> (line: %d, col: %d)", tea_tok_name(token_type),
                token->line, token->col);
  }

  tea_list_add_tail(&self->toks, &token->link);
}

static void skip_whitespaces(tea_lexer_t *self, const char *input)
{
  while (true) {
    const char c = input[self->pos];
    switch (c) {
    case WHITESPACE:
    case CRR:
    case TAB:
      self->col++;
      self->pos++;
      break;
    case EOL:
      self->col = 1;
      self->pos++;
      self->line++;
      break;
    default:
      return;
    }
  }
}

static bool scan_comments(tea_lexer_t *self, const char *input)
{
  if (input[self->pos] == '/') {
    int position = self->pos + 2;
    switch (input[self->pos + 1]) {
    case '*':
      while (true) {
        if (input[position] == 0) {
          self->pos = position;
          return true;
        }
        if (input[position] == EOL) {
          position++;
          self->line++;
          self->col = 1;
        } else if (input[position] == '*' && input[position + 1] == '/') {
          // The section is correct, just return true
          self->pos = position + 2;
          self->col += 2;
          return true;
        } else {
          position++;
          self->col++;
        }
      }
    case '/':
      while (true) {
        if (input[position] == 0) {
          self->pos = position;
          return true;
        }
        if (input[position] == EOL) {
          self->line += 1;
          self->col = 1;
          self->pos = position + 1;
          return true;
        }
        position++;
        self->col++;
      }
    default:
      return false;
    }
  }

  return false;
}

static bool scan_operator(tea_lexer_t *self, const char *input)
{
  int token_length = 1;
  int token_type;

  switch (input[self->pos]) {
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
    if (input[self->pos + 1] == '=') {
      token_type = TEA_TOKEN_EQ;
      token_length = 2;
      break;
    }
    token_type = TEA_TOKEN_ASSIGN;
    break;
  case '!':
    if (input[self->pos + 1] == '=') {
      token_type = TEA_TOKEN_NE;
      token_length = 2;
      break;
    }
    token_type = TEA_TOKEN_EXCLAMATION_MARK;
    break;
  case '-':
    if (input[self->pos + 1] == '>') {
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
  case '&':
    if (input[self->pos + 1] == '&') {
      token_type = TEA_TOKEN_AND;
      token_length = 2;
      break;
    }
    return false;
  case '|':
    if (input[self->pos + 1] == '|') {
      token_type = TEA_TOKEN_OR;
      token_length = 2;
      break;
    }
    return false;
  case '>':
    if (input[self->pos + 1] == '=') {
      token_type = TEA_TOKEN_GE;
      token_length = 2;
      break;
    }
    token_type = TEA_TOKEN_GT;
    break;
  case '<':
    if (input[self->pos + 1] == '=') {
      token_type = TEA_TOKEN_LE;
      token_length = 2;
      break;
    }
    token_type = TEA_TOKEN_LT;
    break;
  case '.':
    token_type = TEA_TOKEN_DOT;
    break;
  case '?':
    token_type = TEA_TOKEN_QUESTION_MARK;
    break;
  default:
    return false;
  }

  create_token(self, token_type, &input[self->pos], token_length);

  self->col += token_length;
  self->pos += token_length;

  return true;
}

static bool scan_number(tea_lexer_t *self, const char *input)
{
  const char first_char = input[self->pos];

  if (isdigit(first_char) ||
      (first_char == '.' && isdigit(input[self->pos + 1]))) {
    const int start_position = self->pos;
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

    char tmp_buffer[32] = { 0 };
    if (length < 32) {
      strncpy(tmp_buffer, buffer, length);
    } else {
      tea_log_err(
        "Lexer error: Number literal too large (exceeds 32 characters) at line %d, column %d",
        self->line, self->col);
      return false;
    }

    char *end;
    if (is_float) {
      float value = strtof(tmp_buffer, &end);
      if (*end == 0) {
        create_token(self, TEA_TOKEN_FLOAT_NUMBER, (char *)&value,
                     sizeof(value));
      } else {
        tea_log_err(
          "Lexer error: Invalid float literal '%s' at line %d, column %d",
          tmp_buffer, self->line, self->col);
        return false;
      }
    } else {
      int value = (int)strtol(tmp_buffer, &end, 10);
      if (*end == 0) {
        create_token(self, TEA_TOKEN_INTEGER_NUMBER, (char *)&value,
                     sizeof(value));
      } else {
        tea_log_err(
          "Lexer error: Invalid integer literal '%s' at line %d, column %d",
          tmp_buffer, self->line, self->col);
        return false;
      }
    }

    self->col += length;
    self->pos = current_position;

    return true;
  }

  return false;
}

#define TEA_STRING_MAX_SIZE 1024

static bool scan_string(tea_lexer_t *self, const char *input)
{
  if (input[self->pos] == '\'') {
    const int start_position = self->pos + 1; // Skip the opening quote
    int current_position = start_position;

    char tmp_string[TEA_STRING_MAX_SIZE] = { 0 };
    int tmp_string_length = 0;

    // Scan until we find the closing quote
    while (true) {
      const char c = input[current_position];

      if (c == EOS) {
        tea_log_err(
          "Lexer error: Unterminated string literal at line %d, column %d",
          self->line, self->col);
        return false;
      }

      if (c == '\'') {
        break;
      }

      if (c == EOL) {
        tea_log_err(
          "Lexer error: String literals cannot contain newline characters at line %d, column %d",
          self->line, self->col);
        return false;
      }

      int shift;
      if (c != '\\') {
        tmp_string[tmp_string_length++] = c;
        shift = 1;
      } else {
        switch (input[current_position + 1]) {
        case '\\':
        case '\'':
        case '\"':
          tmp_string[tmp_string_length++] = input[current_position + 1];
          break;
        case 'n':
          tmp_string[tmp_string_length++] = EOL;
          break;
        case 't':
          tmp_string[tmp_string_length++] = TAB;
          break;
        case 'r':
          tmp_string[tmp_string_length++] = CRR;
          break;
        default:
          break;
        }
        shift = 2;
      }

      self->col += shift;
      current_position += shift;
    }

    create_token(self, TEA_TOKEN_STRING, tmp_string, tmp_string_length);

    self->pos = current_position + 1; // Skip the closing quote
    self->col++;

    return true;
  }

  return false;
}

static bool scan_ident(tea_lexer_t *self, const char *input)
{
  char c = input[self->pos];
  if (isalpha(c) || c == '_') {
    int token_length = 0;

    while (true) {
      c = input[self->pos + token_length];
      if (!isalnum(c) && c != '_') {
        break;
      }

      token_length++;
    }

    const char *token_name = &input[self->pos];
    const int token_type = tea_get_ident_type(token_name, token_length);
    if (token_type != TEA_TOKEN_IDENT) {
      create_token(self, token_type, NULL, 0);
    } else {
      create_token(self, token_type, token_name, token_length);
    }

    self->pos += token_length;
    self->col += token_length;

    return true;
  }

  return false;
}

static void unknown_character(const tea_lexer_t *self, const char *input)
{
  const char c = input[self->pos];
  if (c != EOS) {
    if (c != EOL) {
      tea_log_err(
        "Lexer error: Unknown character '%c' at line %d, column %d, position %d",
        c, self->line, self->col, self->pos);
    } else {
      tea_log_err(
        "Lexer error: Unexpected end of line character at line %d, column %d, position %d",
        self->line, self->col, self->pos);
    }
    exit(1);
  }
}

static struct {
  bool (*scan)(tea_lexer_t *self, const char *input);
} scanners[] = {
  scan_comments, scan_operator, scan_string, scan_number, scan_ident, { NULL },
};

static bool try_lexer_scanners(void *self, const char *input)
{
  for (int i = 0; scanners[i].scan; ++i) {
    if (scanners[i].scan(self, input)) {
      return true;
    }
  }
  return false;
}

void tea_lexer_tokenize(tea_lexer_t *self, const char *input)
{
  while (input[self->pos] != EOS) {
    skip_whitespaces(self, input);

    const bool success = try_lexer_scanners(self, input);
    if (!success) {
      unknown_character(self, input);
    }
  }
}
