#include <rtl_log.h>
#include <string.h>

#include "rtl.h"
#include "tea_ast.h"
#include "tea_interp.h"
#include "tea_parser.h"

void print_usage(const char *program_name)
{
  rtl_log_inf("Usage: %s [options] <tea_file>", program_name);
  rtl_log_inf("Options:");
  rtl_log_inf("  -h, --help     Show this help message");
  rtl_log_inf("  -t, --test     Run with test input");
  rtl_log_inf("");
  rtl_log_inf("Examples:");
  rtl_log_inf("  %s example.tea", program_name);
  rtl_log_inf("  %s --test", program_name);
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

#if 0
    // Execute the script
    rtl_log_inf("Executing script...");
    tea_context_t context;
    tea_interp_init(&context);

    const bool execution_success = tea_interp_execute(&context, ast);
    if (execution_success) {
      rtl_log_inf("Script executed successfully!");
    } else {
      rtl_log_err("Script execution failed!");
    }

    tea_interp_cleanup(&context);
#endif

    tea_ast_node_free(ast);
  }

  rtl_log_dbg("Parsing completed successfully!");

  tea_lexer_cleanup(&lexer);
  rtl_cleanup();

  return 0;
}
