#include "tea_interp.h"

#include "tea_fn.h"
#include "tea_scope.h"
#include "tea_struct.h"

#include <rtl_memory.h>

void tea_interp_init(tea_ctx_t *ctx, const char *fname)
{
  ctx->fname = fname;
  rtl_list_init(&ctx->fns);
  rtl_list_init(&ctx->nfns);
  rtl_list_init(&ctx->structs);

  rtl_list_init(&ctx->vars);
}

void tea_interp_cleanup(const tea_ctx_t *ctx)
{
  rtl_list_entry_t *entry;
  rtl_list_entry_t *safe;

  rtl_list_for_each_safe(entry, safe, &ctx->fns)
  {
    tea_fn_t *function = rtl_list_record(entry, tea_fn_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &ctx->nfns)
  {
    tea_native_fn_t *function = rtl_list_record(entry, tea_native_fn_t, link);
    rtl_list_remove(entry);
    rtl_free(function);
  }

  rtl_list_for_each_safe(entry, safe, &ctx->structs)
  {
    tea_struct_decl_t *struct_declaration =
      rtl_list_record(entry, tea_struct_decl_t, link);
    rtl_list_remove(entry);

    rtl_list_entry_t *function_entry;
    rtl_list_entry_t *function_entry_safe;
    rtl_list_for_each_safe(function_entry, function_entry_safe,
                           &struct_declaration->fns)
    {
      tea_fn_t *function = rtl_list_record(function_entry, tea_fn_t, link);
      rtl_list_remove(function_entry);
      rtl_free(function);
    }

    rtl_free(struct_declaration);
  }

  rtl_list_for_each_safe(entry, safe, &ctx->vars)
  {
    tea_var_t *variable = rtl_list_record(entry, tea_var_t, link);
    rtl_list_remove(entry);
    rtl_free(variable);
  }
}
