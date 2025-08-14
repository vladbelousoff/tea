#include "tea_log.h"
#include "tea_memory.h"
#include <string.h>

#include "tea_ast.h"
#include "tea_lexer.h"

static void *parser_malloc(const size_t size)
{
  return tea_malloc(size);
}

static void parser_free(void *ptr)
{
  tea_free(ptr);
}

extern void *ParseAlloc(void *(*cb_malloc)(size_t));
extern void ParseFree(void *p, void (*cb_free)(void *));
extern void Parse(void *yyp, int yymajor, tea_tok_t *yyminor,
                  tea_node_t **result);

static char *read_file(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file) {
    tea_log_err("Parser error: Cannot open input file '%s' for reading",
                filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  const long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = tea_malloc(file_size + 1);
  if (!buffer) {
    tea_log_err("Parser error: Cannot allocate memory to read file '%s'",
                filename);
    fclose(file);
    return NULL;
  }

  // Read file content
  const size_t bytes_read = fread(buffer, 1, file_size, file);
  buffer[bytes_read] = 0;

  fclose(file);
  return buffer;
}

tea_node_t *tea_parse_string(tea_lexer_t *lexer, const char *input)
{
  void *parser = ParseAlloc(parser_malloc);
  if (!parser) {
    tea_log_err("Parser error: Failed to allocate memory for parser instance");
    return NULL;
  }

  tea_node_t *result = NULL;
  tea_lexer_tokenize(lexer, input);
  const bool is_lexer_empty = tea_list_empty(&lexer->toks);

  tea_list_entry_t *entry;
  tea_list_for_each(entry, &lexer->toks)
  {
    tea_tok_t *token = tea_list_record(entry, tea_tok_t, link);
    Parse(parser, token->type, token, &result);
  }

  if (!is_lexer_empty) {
    Parse(parser, 0, NULL, &result);
  }

  ParseFree(parser, parser_free);

  return result;
}

tea_node_t *tea_parse_file(tea_lexer_t *lexer, const char *filename)
{
  char *content = read_file(filename);
  if (!content) {
    return NULL;
  }

  tea_node_t *result = tea_parse_string(lexer, content);
  tea_free(content);

  return result;
}
