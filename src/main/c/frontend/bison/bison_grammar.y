%{

#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>

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

	Expr *expression;
	LiteralExpr *literal;
  VariableExpr *variable;
  GroupExpr *group;
  UnaryExpr *unary;
  BinaryExpr *binary;
  AssignmentExpr *assignment;
  DeclarationExpr *declaration;
  IfExpr *if_expr;
  ForExpr *for_expr;
  BlockExpr *block_expr;

  Program *program;
  ExprList *expression_list;
}

%destructor { ast_free_expr($$); } <expression>
%destructor { ast_free_literal($$); } <literal>
%destructor { ast_free_unary($$); } <unary>
%destructor { ast_free_binary($$); } <binary>
%destructor { ast_free_group($$); } <group>
%destructor { ast_free_expr_list($$, true); } <expression_list>
%destructor { ast_free_program($$); } <program>

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
%token <token>    AND
%token <token>    OR
%token <token>    NOT
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
%token <token>    END

%type <expression>        expression
%type <literal>           literal
%type <unary>             unary
%type <binary>            binary
%type <group>             group

%type <program>           program
%type <expression_list>   expression_list

// Precedence
%right EQUAL
%left OR
%left AND
%left NOT
%left EQUAL_EQUAL BANG_EQUAL
%nonassoc GREATER_EQUAL LESS_EQUAL GREATER LESS
%left PLUS MINUS
%left STAR SLASH
%left BANG

%%

program: expression_list END                { $$ = parse_program($1); }
  ;

expression_list: expression                 { $$ = parse_expr_list($1, NULL); }
  | expression_list expression              { $$ = parse_expr_list($2, $1); }
  ;

expression: literal                         { $$ = parse_literal_expr($1); }
  | unary                                   { $$ = parse_unary_expr($1); }
  | binary                                  { $$ = parse_binary_expr($1); }
  | group                                   { $$ = parse_group_expr($1); }
  ;

literal: INTEGER                            { $$ = parse_integer_literal($1); }
  | FLOAT                                   { $$ = parse_float_literal($1); }
  | STRING                                  { $$ = parse_string_literal($1); }
  | BOOL                                    { $$ = parse_boolean_literal($1); }
  ;

unary: BANG expression                      { $$ = parse_unary($1, $2); }
  | NOT expression                          { $$ = parse_unary($1, $2); }
  ;

binary: expression PLUS expression          { $$ = parse_binary($1, $2, $3); }
  | expression MINUS expression             { $$ = parse_binary($1, $2, $3); }
  | expression STAR expression              { $$ = parse_binary($1, $2, $3); }
  | expression SLASH expression             { $$ = parse_binary($1, $2, $3); }
  | expression GREATER_EQUAL expression     { $$ = parse_binary($1, $2, $3); }
  | expression GREATER expression           { $$ = parse_binary($1, $2, $3); }
  | expression LESS_EQUAL expression        { $$ = parse_binary($1, $2, $3); }
  | expression LESS expression              { $$ = parse_binary($1, $2, $3); }
  | expression EQUAL_EQUAL expression       { $$ = parse_binary($1, $2, $3); }
  | expression BANG_EQUAL expression        { $$ = parse_binary($1, $2, $3); }
  | expression AND expression               { $$ = parse_binary($1, $2, $3); }
  | expression OR expression                { $$ = parse_binary($1, $2, $3); }
  ;

group: PAREN_L expression PAREN_R           { $$ = parse_group($2); }

%%
