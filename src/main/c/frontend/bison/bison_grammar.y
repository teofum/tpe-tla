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
	TokenLabel token;
	i64 integer;
	f64 floating;
	str string;
	bool boolean;

	i64 constant;
}

%token <token>    NIL

%token <integer>  INTEGER
%token <floating> FLOAT
%token <string>   STRING
%token <boolean>  BOOL

%token <token>    IGNORED
%token <token>    UNKNOWN

%type <constant> constant

%%

constant: INTEGER            { $$ = parse_integer($1); }
  ;

%%
