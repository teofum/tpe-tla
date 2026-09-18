%{

#include <support/types.h>
#include <frontend/frontend.h>

#include "bison_actions.h"
#include "bison_parser.h"

void yyerror(YYLTYPE *location, const char *message) {}

%}

%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	i32 integer;
	TokenLabel token;

	i32 constant;
}

%token <integer> INTEGER

%token <token> IGNORED
%token <token> UNKNOWN

%type <constant> constant

%%

constant: INTEGER            { $$ = parse_integer($1); }
  ;

%%
