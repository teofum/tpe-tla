#ifndef CODEGEN_HEADER
#define CODEGEN_HEADER

#include <support/state.h>

void cg_init();
void cg_shutdown();

void cg_generate(u32 file_idx);

#endif
