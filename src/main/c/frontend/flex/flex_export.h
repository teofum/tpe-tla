#ifndef FLEX_EXPORT_HEADER
#define FLEX_EXPORT_HEADER

#include <frontend/frontend.h>
#include <support/types.h>

/**
 * Hook that allows to export a static function or variable from the inside of
 * Flex infrastructure, in this case, the current context (a.k.a. start
 * condition). This source exists only because Flex uses static for methods
 * that are in its public API, a clearly flawed design decision.
 */

FlexContext flex_current_context(Frontend *f) {
  struct yyguts_t *yyg = (struct yyguts_t *)f->scanner;
  return YYSTATE;
}

void flex_enter_context(Frontend *f, FlexContext ctx) {
  yy_push_state(ctx, f->scanner);
}

void flex_leave_context(Frontend *f) { yy_pop_state(f->scanner); }

bool flex_has_buffer(Frontend *f) {
  struct yyguts_t *yyg = (struct yyguts_t *)f->scanner;
  return YY_CURRENT_BUFFER != NULL;
}

#endif
