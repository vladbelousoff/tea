#include "tea_interp.h"

#include "tea_fn.h"
#include "tea_scope.h"
#include "tea_struct.h"

#include "tea_memory.h"

void tea_interp_init(tea_ctx_t *ctx, const char *fname)
{
  ctx->file_name = fname;
  tea_list_init(&ctx->funcs);
  tea_list_init(&ctx->native_funcs);
  tea_list_init(&ctx->structs);

  tea_list_init(&ctx->vars);
}

void tea_interp_cleanup(const tea_ctx_t *ctx)
{
  tea_list_entry_t *entry;
  tea_list_entry_t *safe;

  tea_list_for_each_safe(entry, safe, &ctx->funcs)
  {
    tea_fn_t *function = tea_list_record(entry, tea_fn_t, link);
    tea_list_remove(entry);
    tea_free(function);
  }

  tea_list_for_each_safe(entry, safe, &ctx->native_funcs)
  {
    tea_native_fn_t *function = tea_list_record(entry, tea_native_fn_t, link);
    tea_list_remove(entry);
    tea_free(function);
  }

  tea_list_for_each_safe(entry, safe, &ctx->structs)
  {
    tea_struct_decl_t *struct_declaration =
      tea_list_record(entry, tea_struct_decl_t, link);
    tea_list_remove(entry);

    tea_list_entry_t *function_entry;
    tea_list_entry_t *function_entry_safe;
    tea_list_for_each_safe(function_entry, function_entry_safe,
                           &struct_declaration->funcs)
    {
      tea_fn_t *function = tea_list_record(function_entry, tea_fn_t, link);
      tea_list_remove(function_entry);
      tea_free(function);
    }

    tea_free(struct_declaration);
  }

  tea_list_for_each_safe(entry, safe, &ctx->vars)
  {
    tea_var_t *variable = tea_list_record(entry, tea_var_t, link);
    tea_list_remove(entry);
    tea_free(variable);
  }
}
