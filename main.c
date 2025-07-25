#include <rtl_log.h>
#include <string.h>

#include "rtl.h"
#include "tea_ast.h"
#include "tea_interpret.h"
#include "tea_parser.h"

void print_usage(const char *program_name)
{
  rtl_log_inf("Usage: %s [options] <tea_file>", program_name);
  rtl_log_inf("Options:");
  rtl_log_inf("  -h, --help     Show this help message");
  rtl_log_inf("");
  rtl_log_inf("Examples:");
  rtl_log_inf("  %s example.tea", program_name);
}

static tea_value_t tea_print(const tea_value_t *args, const int arg_count)
{
  for (int i = 0; i < arg_count; i++) {
    const tea_value_t arg = args[i];
    switch (arg.type) {
      case TEA_VALUE_I32:
        printf("%d", arg.i32);
        break;
      case TEA_VALUE_F32:
        printf("%f", arg.f32);
        break;
      case TEA_VALUE_STRING:
        printf("%s", arg.string);
        break;
      case TEA_VALUE_INSTANCE:
      case TEA_VALUE_INVALID:
        break;
    }
  }

  return tea_value_invalid();
}

int main(const int argc, char *argv[])
{
  const char *filename = NULL;

  rtl_init();

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (argv[i][0] != '-') {
      filename = argv[i];
    } else {
      rtl_log_err("Unknown option: %s", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  // Check if filename was provided
  if (!filename) {
    rtl_log_err("Error: No input file specified");
    print_usage(argv[0]);
    return 1;
  }

  // Check if file exists
  FILE *test_file = fopen(filename, "r");
  if (!test_file) {
    rtl_log_err("Error: Cannot open file '%s'", filename);
    return 1;
  }
  fclose(test_file);

  rtl_log_inf("Parsing file: %s", filename);

  tea_lexer_t lexer;
  tea_lexer_init(&lexer);
  tea_ast_node_t *ast = tea_parse_file(&lexer, filename);

  rtl_log_dbg("Parsing summary:");
  rtl_log_dbg("File: %s", filename);
  rtl_log_dbg("Status: successfully parsed");

  if (ast) {
    tea_ast_node_print(ast, 0);
    rtl_log_dbg("Root node type: %s", ast->type == TEA_AST_NODE_PROGRAM ? "PROGRAM" : "OTHER");
  }

  rtl_log_dbg("Parsing completed successfully!");

  int ret_code = 0;
  if (ast) {
    tea_context_t context;
    tea_interpret_init(&context, filename);

    tea_bind_native_function(&context, "print", tea_print);

    tea_scope_t global_scope;
    tea_scope_init(&global_scope, NULL);
    if (!tea_interpret_execute(&context, &global_scope, ast, NULL, NULL)) {
      ret_code = 1;
    }

    tea_scope_cleanup(&context, &global_scope);
    tea_interpret_cleanup(&context);

    tea_ast_node_free(ast);
  }

  tea_lexer_cleanup(&lexer);
  rtl_cleanup();

  return ret_code;
}
