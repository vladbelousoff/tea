#include "tea.h"
#include "tea_log.h"

#include <string.h>

#include "tea_ast.h"
#include "tea_fn.h"
#include "tea_interp.h"
#include "tea_parser.h"
#include "tea_stmt.h"

void print_usage(const char *program_name)
{
  tea_log_inf("Usage: %s [options] <tea_file>", program_name);
  tea_log_inf("Options:");
  tea_log_inf("  -h, --help     Show this help message");
  tea_log_inf("");
  tea_log_inf("Examples:");
  tea_log_inf("  %s example.tea", program_name);
}

static tea_val_t tea_print(tea_fn_args_t *args)
{
  for (;;) {
    tea_var_t *arg = tea_fn_args_pop(args);
    if (!arg) {
      break;
    }

    const tea_val_t value = arg->val;
    switch (value.type) {
    case TEA_V_NULL:
      printf("null");
    case TEA_V_I32:
      printf("%d", value.i32);
      break;
    case TEA_V_F32:
      printf("%f", value.f32);
      break;
    case TEA_V_INST:
      if (!strcmp(value.obj->type, "string")) {
        printf("%s", (char *)value.obj->buf);
      }
      break;
    case TEA_V_UNDEF:
      break;
    }
  }

  return tea_val_undef();
}

static tea_val_t tea_println(tea_fn_args_t *args)
{
  const tea_val_t value = tea_print(args);

  static const char newline = '\n';
  fwrite(&newline, 1, 1, stdout);

  return value;
}

int main(const int argc, char *argv[])
{
  const char *filename = NULL;

  tea_init(NULL, NULL);

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (argv[i][0] != '-') {
      filename = argv[i];
    } else {
      tea_log_err("Unknown option: %s", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  // Check if filename was provided
  if (!filename) {
    tea_log_err("Error: No input file specified");
    print_usage(argv[0]);
    return 1;
  }

  // Check if file exists
  FILE *test_file = fopen(filename, "r");
  if (!test_file) {
    tea_log_err("Error: Cannot open file '%s'", filename);
    return 1;
  }
  fclose(test_file);

  tea_log_inf("Parsing file: %s", filename);

  tea_lexer_t lexer;
  tea_lexer_init(&lexer);
  tea_node_t *ast = tea_parse_file(&lexer, filename);

  tea_log_dbg("Parsing summary:");
  tea_log_dbg("File: %s", filename);
  tea_log_dbg("Status: successfully parsed");

  if (ast) {
    tea_node_print(ast, 0);
    tea_log_dbg("Root node type: %s",
                ast->type == TEA_N_PROG ? "PROGRAM" : "OTHER");
  }

  tea_log_dbg("Parsing completed successfully!");

  int ret_code = 0;
  if (ast) {
    tea_ctx_t context;
    tea_interp_init(&context, filename);

    tea_bind_native_fn(&context, "print", tea_print);
    tea_bind_native_fn(&context, "println", tea_println);

    tea_scope_t global_scope;
    tea_scope_init(&global_scope, NULL);
    if (!tea_exec(&context, &global_scope, ast, NULL, NULL)) {
      ret_code = 1;
    }

    tea_scope_cleanup(&context, &global_scope);
    tea_interp_cleanup(&context);

    tea_node_free(ast);
  }

  tea_lexer_cleanup(&lexer);
  tea_cleanup();

  return ret_code;
}
