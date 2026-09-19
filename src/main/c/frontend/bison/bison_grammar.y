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

// Symbols
%token <token>    PAREN_L
%token <token>    PAREN_R
%token <token>    SQUARE_L
%token <token>    SQUARE_R
%token <token>    CURLY_L
%token <token>    CURLY_R
%token <token>    COLON
%token <token>    COMMA
%token <token>    DOT
%token <token>    MINUS
%token <token>    PLUS
%token <token>    STAR
%token <token>    SLASH
%token <token>    QUESTION_MARK
%token <token>    HASH
%token <token>    EQUAL
%token <token>    EQUAL_EQUAL
%token <token>    BANG
%token <token>    BANG_EQUAL
%token <token>    GREATER
%token <token>    GREATER_EQUAL
%token <token>    LESS
%token <token>    LESS_EQUAL

// Keywords
%token <token>    IF
%token <token>    ELSE
%token <token>    FOR
%token <token>    IN
%token <token>    NIL
%token <token>    IS
%token <token>    OF
%token <token>    STRUCT
%token <token>    UNION
%token <token>    ENUM
%token <token>    FUNCTION
%token <token>    IMPORT

// Identifiers
%token <string>   IDENTIFIER

// Literals
%token <integer>  INTEGER
%token <floating> FLOAT
%token <string>   STRING
%token <boolean>  BOOL

// Other
%token <token>    IGNORED
%token <token>    UNKNOWN

%type <constant>  constant

%%

constant: INTEGER            { $$ = parse_integer($1); }
  ;

%%
