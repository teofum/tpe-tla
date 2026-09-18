%top{

#include "../../support/types.h"
#include "../token.h"
#include "../lexer.h"
#include "../bison/bison_parser.h"

FlexContext flex_current_context(LexicalAnalyzer *la);
void flex_enter_context(LexicalAnalyzer *la, FlexContext ctx);
void flex_leave_context(LexicalAnalyzer *la);
bool flex_has_buffer(LexicalAnalyzer *la);

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

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "flex_actions.h"
#include "flex_export.h"
