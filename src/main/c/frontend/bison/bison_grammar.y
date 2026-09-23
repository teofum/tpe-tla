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
	TokenMeta *token;
	IntegerLiteral *integer;
	FloatLiteral *floating;
	StringLiteral *string;
	BooleanLiteral *boolean;

	Stmt *statement;
  DeclarationStmt *declaration;
  TypeAliasStmt *type_alias;

	Expr *expression;
	LiteralExpr *literal;
  VariableExpr *variable;
  GroupExpr *group;
  UnaryExpr *unary;
  BinaryExpr *binary;
  AssignmentExpr *assignment;
  IfExpr *if_expr;
  ForExpr *for_expr;
  BlockExpr *block;

  Type *type;
  StructField *struct_field;

  Program *program;
  StmtList *statement_list;
  StructFieldList *struct_field_list;
  TypeList *type_list;
  IdentifierList *identifier_list;
}

%destructor { ast_free_token($$); } <token>
%destructor { ast_free_int_literal($$); } <integer>
%destructor { ast_free_float_literal($$); } <floating>
%destructor { ast_free_string_literal($$); } <string>
%destructor { ast_free_bool_literal($$); } <boolean>
%destructor { ast_free_stmt($$); } <statement>
%destructor { ast_free_expr($$); } <expression>
%destructor { ast_free_literal($$); } <literal>
%destructor { ast_free_variable($$); } <variable>
%destructor { ast_free_unary($$); } <unary>
%destructor { ast_free_binary($$); } <binary>
%destructor { ast_free_group($$); } <group>
%destructor { ast_free_assignment($$); } <assignment>
%destructor { ast_free_if($$); } <if_expr>
%destructor { ast_free_for($$); } <for_expr>
%destructor { ast_free_block($$); } <block>
%destructor { ast_free_decl($$); } <declaration>
%destructor { ast_free_alias($$); } <type_alias>
%destructor { ast_free_type($$); } <type>
%destructor { ast_free_stmt_list($$, true); } <statement_list>
%destructor { ast_free_struct_field_list($$, true); } <struct_field_list>
%destructor { ast_free_struct_field($$); } <struct_field>
%destructor { ast_free_type_list($$, true); } <type_list>
%destructor { ast_free_identifier_list($$, true); } <identifier_list>
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
%token <token>    IDENTIFIER

// Literals
%token <integer>  INTEGER
%token <floating> FLOAT
%token <string>   STRING
%token <boolean>  BOOL

// Other
%token <token>    IGNORED
%token <token>    UNKNOWN
%token <token>    END

%type <statement>         statement
%type <declaration>       declaration
%type <type_alias>        type_alias

%type <expression>        expression
%type <literal>           literal
%type <variable>          variable
%type <unary>             unary
%type <binary>            binary
%type <group>             group
%type <assignment>        assignment
%type <if_expr>           if_expr
%type <for_expr>          for_expr
%type <block>             block

%type <type>              type
%type <struct_field>      struct_field

%type <program>           program
%type <statement_list>    statement_list
%type <struct_field_list> struct_field_list
%type <type_list>         type_list
%type <identifier_list>   identifier_list

// Precedence
%left FOR IN
%left IF
%left ELSE
%right EQUAL
%left OR
%left AND
%left NOT
%left QUESTION_MARK
%left SQUARE_L SQUARE_R
%left EQUAL_EQUAL BANG_EQUAL
%nonassoc GREATER_EQUAL LESS_EQUAL GREATER LESS
%left PLUS MINUS
%left STAR SLASH
%left BANG
%left DOT

%%

program: statement_list END                                         { $$ = parse_program($1); }
  ;

statement_list: statement                                           { $$ = parse_stmt_list($1, NULL); }
  | statement_list statement                                        { $$ = parse_stmt_list($2, $1); }
  ;

statement: expression                                               { $$ = parse_expr_stmt($1); }
  | declaration                                                     { $$ = parse_declaration_stmt($1); }
  | type_alias                                                      { $$ = parse_type_alias_stmt($1); }
  ;

declaration: IDENTIFIER COLON type EQUAL expression                 { $$ = parse_declaration($1, $2, $3, $4, $5); }
  | IDENTIFIER COLON EQUAL expression                               { $$ = parse_declaration($1, $2, NULL, $3, $4); }
  ;

type_alias: IDENTIFIER IS type                                      { $$ = parse_type_alias($1, $2, $3); }
  ;

expression: literal                                                 { $$ = parse_literal_expr($1); }
  | variable                                                        { $$ = parse_variable_expr($1); }
  | unary                                                           { $$ = parse_unary_expr($1); }
  | binary                                                          { $$ = parse_binary_expr($1); }
  | group                                                           { $$ = parse_group_expr($1); }
  | assignment                                                      { $$ = parse_assignment_expr($1); }
  | if_expr                                                         { $$ = parse_if_expr($1); }
  | for_expr                                                        { $$ = parse_for_expr($1); }
  | block                                                           { $$ = parse_block_expr($1); }
  ;

literal: INTEGER                                                    { $$ = parse_integer_literal($1); }
  | FLOAT                                                           { $$ = parse_float_literal($1); }
  | STRING                                                          { $$ = parse_string_literal($1); }
  | BOOL                                                            { $$ = parse_boolean_literal($1); }
  ;

variable: IDENTIFIER                                                { $$ = parse_named_variable($1); }
  | variable DOT IDENTIFIER                                         { $$ = parse_struct_member_variable($1, $2, $3); }
  | variable SQUARE_L expression SQUARE_R                           { $$ = parse_indexed_variable($1, $2, $3, $4); }
  ;

unary: BANG expression                                              { $$ = parse_unary($1, $2); }
  | NOT expression                                                  { $$ = parse_unary($1, $2); }
  ;

binary: expression PLUS expression                                  { $$ = parse_binary($1, $2, $3); }
  | expression MINUS expression                                     { $$ = parse_binary($1, $2, $3); }
  | expression STAR expression                                      { $$ = parse_binary($1, $2, $3); }
  | expression SLASH expression                                     { $$ = parse_binary($1, $2, $3); }
  | expression GREATER_EQUAL expression                             { $$ = parse_binary($1, $2, $3); }
  | expression GREATER expression                                   { $$ = parse_binary($1, $2, $3); }
  | expression LESS_EQUAL expression                                { $$ = parse_binary($1, $2, $3); }
  | expression LESS expression                                      { $$ = parse_binary($1, $2, $3); }
  | expression EQUAL_EQUAL expression                               { $$ = parse_binary($1, $2, $3); }
  | expression BANG_EQUAL expression                                { $$ = parse_binary($1, $2, $3); }
  | expression AND expression                                       { $$ = parse_binary($1, $2, $3); }
  | expression OR expression                                        { $$ = parse_binary($1, $2, $3); }
  ;

group: PAREN_L expression PAREN_R                                   { $$ = parse_group($1, $2, $3); }
  ;

assignment: variable EQUAL expression                               { $$ = parse_assignment($1, $2, $3); }
  ;

if_expr: IF expression expression ELSE expression                   { $$ = parse_if($1, $2, $3, $4, $5); }
  | IF expression expression                                        { $$ = parse_if($1, $2, $3, NULL, NULL); }
  ;

for_expr: FOR IDENTIFIER COMMA IDENTIFIER IN expression expression  { $$ = parse_for($1, $2, $3, $4, $5, $6, $7); }
  | FOR IDENTIFIER IN expression expression                         { $$ = parse_for($1, $2, NULL, NULL, $3, $4, $5); }
  ;

block: CURLY_L statement_list expression CURLY_R                    { $$ = parse_block($1, $2, $3, $4); }
  | CURLY_L expression CURLY_R                                      { $$ = parse_block($1, NULL, $2, $3); }
  ;

type: IDENTIFIER                                                    { $$ = parse_named_type($1); }
  | SQUARE_L SQUARE_R type                                          { $$ = parse_list_type($1, $2, $3); }
  | SQUARE_L type SQUARE_R type                                     { $$ = parse_map_type($1, $2, $3, $4); }
  | STRUCT CURLY_L struct_field_list CURLY_R                        { $$ = parse_struct_type($1, $2, $3, $4, NULL); }
  | STRUCT CURLY_L struct_field_list COMMA CURLY_R                  { $$ = parse_struct_type($1, $2, $3, $5, $4); }
  | UNION CURLY_L type_list CURLY_R                                 { $$ = parse_union_type($1, $2, $3, $4, NULL); }
  | UNION CURLY_L type_list COMMA CURLY_R                           { $$ = parse_union_type($1, $2, $3, $5, $4); }
  | type QUESTION_MARK                                              { $$ = parse_optional_type($1, $2); }
  | LESS type_list GREATER                                          { $$ = parse_tuple_type($1, $2, $3, NULL); }
  | LESS type_list COMMA GREATER                                    { $$ = parse_tuple_type($1, $2, $4, $3); }
  | ENUM CURLY_L identifier_list CURLY_R                            { $$ = parse_enum_type($1, $2, $3, $4, NULL); }
  | ENUM CURLY_L identifier_list COMMA CURLY_R                      { $$ = parse_enum_type($1, $2, $3, $5, $4); }
  | NIL                                                             { $$ = parse_nil_type($1); }
  ;

struct_field_list: struct_field                                     { $$ = parse_struct_field_list($1, NULL, NULL); }
  | struct_field_list COMMA struct_field                            { $$ = parse_struct_field_list($3, $2, $1); }
  ;

struct_field: IDENTIFIER COLON type                                 { $$ = parse_struct_field($1, $2, $3, NULL, NULL); }
  | IDENTIFIER COLON type EQUAL expression                          { $$ = parse_struct_field($1, $2, $3, $4, $5); }
  ;

type_list: type                                                     { $$ = parse_type_list($1, NULL, NULL); }
  | type_list COMMA type                                            { $$ = parse_type_list($3, $2, $1); }
  ;

identifier_list: IDENTIFIER                                         { $$ = parse_identifier_list($1, NULL, NULL); }
  | identifier_list COMMA IDENTIFIER                                { $$ = parse_identifier_list($3, $2, $1); }
  ;

%%
