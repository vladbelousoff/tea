#pragma once

#include "tea_scope.h"

void tea_interpret_init(tea_context_t* context, const char* filename);
void tea_interpret_cleanup(const tea_context_t* context);
