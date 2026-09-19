%top{

#include "../../support/types.h"
#include "../frontend.h"
#include "../bison/bison_parser.h"
#include "flex_actions.h"

FlexContext flex_current_context(Frontend *la);
void flex_enter_context(Frontend *la, FlexContext ctx);
void flex_leave_context(Frontend *la);
bool flex_has_buffer(Frontend *la);

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 0; \
        } \
        else { \
            yylloc->last_column++; \
        } \
    }

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
%x STRING_LITERAL

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+
d               [[:digit:]]
d_nz            [1-9]
d_hex           [[:digit:]]
d_hex_nz        [1-9]
int             (({d_nz}{d}*)|0)

%%

%{
/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */
%}

"if"                          { return lex_keyword(IF); }
"else"                        { return lex_keyword(ELSE); }
"for"                         { return lex_keyword(FOR); }
"in"                          { return lex_keyword(IN); }
"nil"                         { return lex_keyword(NIL); }
"is"                          { return lex_keyword(IS); }
"of"                          { return lex_keyword(OF); }
"struct"                      { return lex_keyword(STRUCT); }
"union"                       { return lex_keyword(UNION); }
"enum"                        { return lex_keyword(ENUM); }
"function"                    { return lex_keyword(FUNCTION); }
"import"                      { return lex_keyword(IMPORT); }

-?{int}                       { return lex_integer_literal(); }
-?{int}?\.{d}+                { return lex_float_literal(); }
-?{d}(\.{d}+)?e(-|\+)?{int}   { return lex_float_literal(); }
\"([^"]|\\\")*\"              { return lex_string_literal(); }

{whitespace}                  { return lex_ignored(); }
.                             { return lex_unknown(); }

<<EOF>>                       { return lex_eof(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "flex_export.h"
