#include "tea_lexer.h"

#include "tea.h"
#include "tea_token.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <rtl_log.h>
#include <rtl_memory.h>

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

  if (token_type == TEA_TOKEN_STRING) {
    rtl_log_dbg("Token: <STRING> (line: %d, col: %d)", token->line, token->column);
  } else if (token_type == TEA_TOKEN_IDENT) {
    rtl_log_dbg("Token: %.*s (line: %d, col: %d)", buffer_size, buffer, token->line, token->column);
  } else if (token_type == TEA_TOKEN_INTEGER_NUMBER) {
    rtl_log_dbg("Token: %d (line: %d, col: %d)", *(int *)buffer, token->line, token->column);
  } else if (token_type == TEA_TOKEN_FLOAT_NUMBER) {
    rtl_log_dbg("Token: %f (line: %d, col: %d)", *(float *)buffer, token->line, token->column);
  } else {
    rtl_log_dbg("Token: <%s> (line: %d, col: %d)", tea_token_get_name(token_type), token->line,
      token->column);
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

static bool scan_comments(tea_lexer_t *self, const char *input)
{
  if (input[self->position] == '/') {
    int position = self->position + 2;
    switch (input[self->position + 1]) {
      case '*':
        while (true) {
          if (input[position] == 0) {
            self->position = position;
            return true;
          }
          if (input[position] == EOL) {
            position++;
            self->line++;
            self->column = 1;
          } else if (input[position] == '*' && input[position + 1] == '/') {
            // The section is correct, just return true
            self->position = position + 2;
            self->column += 2;
            return true;
          } else {
            position++;
            self->column++;
          }
        }
      case '/':
        while (true) {
          if (input[position] == 0) {
            self->position = position;
            return true;
          }
          if (input[position] == EOL) {
            self->line += 1;
            self->column = 1;
            self->position = position + 1;
            return true;
          }
          position++;
          self->column++;
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
      if (input[self->position + 1] == '=') {
        token_type = TEA_TOKEN_EQ;
        token_length = 2;
        break;
      }
      token_type = TEA_TOKEN_ASSIGN;
      break;
    case '!':
      if (input[self->position + 1] == '=') {
        token_type = TEA_TOKEN_NE;
        token_length = 2;
        break;
      }
      return false;
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
    case '&':
      if (input[self->position + 1] == '&') {
        token_type = TEA_TOKEN_AND;
        token_length = 2;
        break;
      }
      return false;
    case '|':
      if (input[self->position + 1] == '|') {
        token_type = TEA_TOKEN_OR;
        token_length = 2;
        break;
      }
      return false;
    case '>':
      if (input[self->position + 1] == '=') {
        token_type = TEA_TOKEN_GE;
        token_length = 2;
        break;
      }
      token_type = TEA_TOKEN_GT;
      break;
    case '<':
      if (input[self->position + 1] == '=') {
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

    char tmp_buffer[32] = { 0 };
    if (length < 32) {
      strncpy(tmp_buffer, buffer, length);
    } else {
      rtl_log_err("The number is too big! Line %d, col %d", self->line, self->column);
      return true;
    }

    char *end;
    if (is_float) {
      float value = strtof(tmp_buffer, &end);
      if (*end == 0) {
        create_token(self, TEA_TOKEN_FLOAT_NUMBER, (char *)&value, sizeof(value));
      } else {
        rtl_log_err("Error converting %s to a f32!", tmp_buffer);
      }
    } else {
      int value = (int)strtol(tmp_buffer, &end, 10);
      if (*end == 0) {
        create_token(self, TEA_TOKEN_INTEGER_NUMBER, (char *)&value, sizeof(value));
      } else {
        rtl_log_err("Error converting %s to a i32!", tmp_buffer);
      }
    }

    self->column += length;
    self->position = current_position;

    return true;
  }

  return false;
}

#define TEA_STRING_MAX_SIZE 1024

static bool scan_string(tea_lexer_t *self, const char *input)
{
  if (input[self->position] == '\'') {
    const int start_position = self->position + 1;  // Skip the opening quote
    int current_position = start_position;

    char tmp_string[TEA_STRING_MAX_SIZE] = { 0 };
    int tmp_string_length = 0;

    // Scan until we find the closing quote
    while (true) {
      const char c = input[current_position];

      if (c == EOS) {
        rtl_log_err("Unterminated string literal at line %d, column %d", self->line, self->column);
        exit(1);
      }

      if (c == '\'') {
        break;
      }

      if (c == EOL) {
        rtl_log_err("Cannot process strings with \\n, line: %d, col: %d", self->line, self->column);
        exit(1);
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

      self->column += shift;
      current_position += shift;
    }

    create_token(self, TEA_TOKEN_STRING, tmp_string, tmp_string_length);

    self->position = current_position + 1;  // Skip the closing quote
    self->column++;

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
    if (c != EOL) {
      rtl_log_err("Unknown character: %c, line: %d, column: %d, position: %d", c, self->line,
        self->column, self->position);
    } else {
      rtl_log_err("Unknown character: <EOL>, line: %d, column: %d, position: %d", self->line,
        self->column, self->position);
    }
    exit(1);
  }
}

static struct
{
  bool (*scan)(tea_lexer_t *self, const char *input);
} scanners[] = {
  scan_comments,
  scan_operator,
  scan_string,
  scan_number,
  scan_ident,
  { NULL },
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
  while (input[self->position] != EOS) {
    skip_whitespaces(self, input);

    const bool success = try_lexer_scanners(self, input);
    if (!success) {
      unknown_character(self, input);
    }
  }
}
