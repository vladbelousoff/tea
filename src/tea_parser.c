#include <rtl_log.h>
#include <rtl_memory.h>
#include <string.h>

#include "tea_ast.h"
#include "tea_lexer.h"

static void *parser_malloc(const size_t size)
{
  return rtl_malloc(size);
}

static void parser_free(void *ptr)
{
  rtl_free(ptr);
}

extern void *ParseAlloc(void *(*cb_malloc)(size_t));
extern void ParseFree(void *p, void (*cb_free)(void *));
extern void Parse(void *yyp, int yymajor, tea_token_t *yyminor, tea_ast_node_t **result);

static char *read_file(const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file) {
    rtl_log_err("Cannot open file '%s'\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  const long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = rtl_malloc(file_size + 1);
  if (!buffer) {
    rtl_log_err("Cannot allocate memory for file '%s'\n", filename);
    fclose(file);
    return NULL;
  }

  // Read file content
  const size_t bytes_read = fread(buffer, 1, file_size, file);
  buffer[bytes_read] = 0;

  fclose(file);
  return buffer;
}

tea_ast_node_t *tea_parse_string(tea_lexer_t *lexer, const char *input)
{
  void *parser = ParseAlloc(parser_malloc);
  if (!parser) {
    rtl_log_err("Failed to create parser!\n");
    return NULL;
  }

  tea_ast_node_t *result = NULL;
  tea_lexer_tokenize(lexer, input);

  rtl_list_entry_t *entry;
  rtl_list_for_each(entry, &lexer->tokens)
  {
    tea_token_t *token = rtl_list_record(entry, tea_token_t, link);
    Parse(parser, token->type, token, &result);
  }

  Parse(parser, 0, NULL, &result);
  ParseFree(parser, parser_free);

  return result;
}

tea_ast_node_t *tea_parse_file(tea_lexer_t *lexer, const char *filename)
{
  char *content = read_file(filename);
  if (!content) {
    return NULL;
  }

  tea_ast_node_t *result = tea_parse_string(lexer, content);
  rtl_free(content);

  return result;
}
