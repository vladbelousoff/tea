#pragma once

#include "tea_scope.h"

void tea_interp_init(tea_ctx_t* ctx, const char* fname);
void tea_interp_cleanup(const tea_ctx_t* ctx);
