#ifndef FLEX_ACTIONS_HEADER
#define FLEX_ACTIONS_HEADER

#include <frontend/frontend.h>
#include <support/types.h>

CompilationStatus lex_keyword(TokenLabel token);
CompilationStatus lex_operator(TokenLabel token);

CompilationStatus lex_integer_literal();
CompilationStatus lex_float_literal();
CompilationStatus lex_string_literal();

CompilationStatus lex_ignored();
CompilationStatus lex_unknown();
CompilationStatus lex_eof();

#endif
