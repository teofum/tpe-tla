%top{

#include "../../support/types.h"
#include "../frontend.h"
#include "../bison/bison_parser.h"
#include "flex_actions.h"

FlexContext flex_current_context(Frontend *la);
void flex_enter_context(Frontend *la, FlexContext ctx);
void flex_leave_context(Frontend *la);
bool flex_has_buffer(Frontend *la);

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+

%%

%{
/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */
%}

[[:digit:]]+          { return lex_integer_literal(); }
.                     { return lex_unknown(); }

<<EOF>>               { return lex_eof(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "flex_export.h"
