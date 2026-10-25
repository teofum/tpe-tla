#ifndef STATIC_ANALYSIS_HEADER
#define STATIC_ANALYSIS_HEADER

#include <ast/ast.h>
#include <ast/symbols.h>
#include <support/state.h>
#include <support/types.h>

void sa_init(CompilerState *cs);
void sa_shutdown();

CompilationStatus sa_build_static_tables(i32 file_idx);
CompilationStatus sa_build_tables_and_typecheck(i32 file_idx);

void sa_free_symbols(Symbols *s);

#endif
