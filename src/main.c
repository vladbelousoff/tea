#include <rtl_log.h>
#include <string.h>

#include "rtl.h"
#include "tea_ast.h"
#include "tea_parser.h"
#include "tea_interp.h"

void print_usage(const char *program_name)
{
  rtl_log_inf("Usage: %s [options] <tea_file>\n", program_name);
  rtl_log_inf("Options:\n");
  rtl_log_inf("  -h, --help     Show this help message\n");
  rtl_log_inf("  -t, --test     Run with test input\n");
  rtl_log_inf("\n");
  rtl_log_inf("Examples:\n");
  rtl_log_inf("  %s example.tea\n", program_name);
  rtl_log_inf("  %s --test\n", program_name);
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
      rtl_log_err("Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  // Check if filename was provided
  if (!filename) {
    rtl_log_err("Error: No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }

  // Check if file exists
  FILE *test_file = fopen(filename, "r");
  if (!test_file) {
    rtl_log_err("Error: Cannot open file '%s'\n", filename);
    return 1;
  }
  fclose(test_file);

  rtl_log_inf("Parsing file: %s\n", filename);

  tea_lexer_t lexer;
  tea_lexer_init(&lexer);
  tea_ast_node_t *ast = tea_parse_file(&lexer, filename);

  if (ast) {
    tea_ast_node_print(ast, 0);

    rtl_log_inf("Parsing summary:\n");
    rtl_log_inf("File: %s\n", filename);
    rtl_log_inf("Status: successfully parsed\n");
    rtl_log_inf("Root node type: %s\n", ast->type == TEA_AST_NODE_PROGRAM ? "PROGRAM" : "OTHER");

    rtl_log_inf("Parsing completed successfully!\n");
    
    // Execute the script
    rtl_log_inf("Executing script...\n");
    tea_context_t context;
    tea_interp_init(&context);
    
    bool execution_success = tea_interp_execute(&context, ast);
    
    if (execution_success) {
      rtl_log_inf("Script executed successfully!\n");
    } else {
      rtl_log_err("Script execution failed!\n");
    }
    
    tea_interp_cleanup(&context);
    tea_ast_node_free(ast);

    tea_lexer_cleanup(&lexer);
    rtl_cleanup();

    return execution_success ? 0 : 1;
  }

  rtl_log_inf("File: %s\n", filename);
  rtl_log_err("Status: failed to parse\n");
  rtl_log_inf("Check the error messages above for details.\n");

  tea_lexer_cleanup(&lexer);
  rtl_cleanup();

  return 1;
}
