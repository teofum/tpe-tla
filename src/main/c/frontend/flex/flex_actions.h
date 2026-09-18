#ifndef FLEX_ACTIONS_HEADER
#define FLEX_ACTIONS_HEADER

#include <frontend/frontend.h>
#include <support/types.h>

CompilationStatus lex_integer_literal();
CompilationStatus lex_unknown();
CompilationStatus lex_eof();

#endif
