#ifndef FLEX_EXPORT_HEADER
#define FLEX_EXPORT_HEADER

#include <frontend/lexer.h>
#include <support/types.h>

/**
 * Hook that allows to export a static function or variable from the inside of
 * Flex infrastructure, in this case, the current context (a.k.a. start
 * condition). This source exists only because Flex uses static for methods
 * that are in its public API, a clearly flawed design decision.
 */

FlexContext flex_current_context(LexicalAnalyzer *la) {
  struct yyguts_t *yyg = (struct yyguts_t *)la->scanner;
  return YYSTATE;
}

void flex_enter_context(LexicalAnalyzer *la, FlexContext ctx) {
  yy_push_state(ctx, la->scanner);
}

void flex_leave_context(LexicalAnalyzer *la) { yy_pop_state(la->scanner); }

bool flex_has_buffer(LexicalAnalyzer *la) {
  struct yyguts_t *yyg = (struct yyguts_t *)la->scanner;
  return YY_CURRENT_BUFFER != NULL;
}

#endif
