%{

#include <ast/ast.h>
#include <frontend/frontend.h>
#include <support/types.h>
#include <support/util.h>

#include "bison_actions.h"
#include "bison_parser.h"

static void yyerror(YYLTYPE *location, const char *message) {
  parse_error((Location *) location, message);
}

%}

%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error custom
%locations

%union {
	TokenLabel token;
	Identifier *identifier;
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
  StructLiteralField *struct_literal_field;
  MapEntry *map_entry;

  Program *program;
  StmtList *statement_list;
  StructFieldList *struct_field_list;
  StructLiteralFieldList *struct_literal_field_list;
  MapEntryList *map_entry_list;
  TypeList *type_list;
  IdentifierList *identifier_list;
  ExprList *expression_list;
}

%destructor { ast_free_identifier($$); } <identifier>
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
%destructor { ast_free_struct_literal_field_list($$, true); } <struct_literal_field_list>
%destructor { ast_free_struct_literal_field($$); } <struct_literal_field>
%destructor { ast_free_map_entry_list($$, true); } <map_entry_list>
%destructor { ast_free_map_entry($$); } <map_entry>
%destructor { ast_free_type_list($$, true); } <type_list>
%destructor { ast_free_identifier_list($$, true); } <identifier_list>
%destructor { ast_free_expr_list($$, true); } <expression_list>
%destructor { ast_free_program($$); } <program>

// Symbols
%token <token>    PAREN_L           "'('"
%token <token>    PAREN_R           "')'"
%token <token>    SQUARE_L          "'['"
%token <token>    SQUARE_R          "']'"
%token <token>    CURLY_L           "'{'"
%token <token>    CURLY_R           "'}'"
%token <token>    COLON             "':'"
%token <token>    COMMA             "','"
%token <token>    DOT               "'.'"
%token <token>    MINUS             "'-'"
%token <token>    PLUS              "'+'"
%token <token>    STAR              "'*'"
%token <token>    SLASH             "'/'"
%token <token>    QUESTION_MARK     "'?'"
%token <token>    HASH              "'#'"
%token <token>    EQUAL             "'='"
%token <token>    EQUAL_EQUAL       "'=='"
%token <token>    BANG              "'!'"
%token <token>    BANG_EQUAL        "'!='"
%token <token>    GREATER           "'>'"
%token <token>    GREATER_EQUAL     "'>='"
%token <token>    LESS              "'<'"
%token <token>    LESS_EQUAL        "'<='"

// Keywords
%token <token>    IF                "'if'"
%token <token>    ELSE              "'else'"
%token <token>    FOR               "'for'"
%token <token>    IN                "'in'"
%token <token>    NIL               "'nil'"
%token <token>    IS                "'is'"
%token <token>    OF                "'of'"
%token <token>    AND               "'and'"
%token <token>    OR                "'or'"
%token <token>    NOT               "'not'"
%token <token>    STRUCT            "'struct'"
%token <token>    UNION             "'union'"
%token <token>    ENUM              "'enum'"
%token <token>    FUNCTION          "'function'"
%token <token>    IMPORT            "'import'"

// Identifiers
%token <identifier> IDENTIFIER      "identifier"

// Literals
%token <integer>  INTEGER           "integer literal"
%token <floating> FLOAT             "float literal"
%token <string>   STRING            "string literal"
%token <boolean>  BOOL              "boolean literal"

// Newline: statement terminator
%token <token>    NL                "<newline>"

// Other
%token <token>    IGNORED
%token <token>    UNKNOWN           "<unknown lexeme>"
%token <token>    END               "<EOF>"

%type <statement>                   statement
%type <declaration>                 declaration
%type <type_alias>                  type_alias

%type <expression>                  expression
%type <literal>                     literal
%type <variable>                    variable
%type <unary>                       unary
%type <binary>                      binary
%type <group>                       group
%type <assignment>                  assignment
%type <if_expr>                     if_expr
%type <for_expr>                    for_expr
%type <block>                       block

%type <type>                        type
%type <struct_field>                struct_field
%type <struct_literal_field>        struct_literal_field
%type <map_entry>                   map_entry

%type <program>                     program
%type <statement_list>              statement_list
%type <struct_field_list>           struct_field_list
%type <struct_literal_field_list>   struct_literal_field_list
%type <map_entry_list>              map_entry_list
%type <type_list>                   type_list
%type <identifier_list>             identifier_list
%type <expression_list>             expression_list

// Precedence
%left NL
%left COMMA
%left FOR IN
%left IF
%left ELSE
%right EQUAL
%left OR
%left AND
%left NOT
%left PAREN_L PAREN_R
%left QUESTION_MARK
%left SQUARE_L SQUARE_R
%left EQUAL_EQUAL BANG_EQUAL
%nonassoc GREATER_EQUAL LESS_EQUAL GREATER LESS
%left PLUS MINUS
%left STAR SLASH
%left BANG
%left DOT

%%

program: statement_list NL END                                      { $$ = parse_program($1); }
  ;

statement_list: statement                                           { $$ = parse_stmt_list($1, NULL); }
  | statement_list NL statement                                     { $$ = parse_stmt_list($3, $1); }
  ;

statement: expression                                               { $$ = parse_expr_stmt($1); }
  | declaration                                                     { $$ = parse_declaration_stmt($1); }
  | type_alias                                                      { $$ = parse_type_alias_stmt($1); }
  | error                                                           { $$ = parse_error_stmt(); }
  ;

declaration: IDENTIFIER COLON type EQUAL expression                 { $$ = parse_declaration($1, $3, $5); }
  | IDENTIFIER COLON EQUAL expression                               { $$ = parse_declaration($1, NULL, $4); }
  ;

type_alias: IDENTIFIER IS type                                      { $$ = parse_type_alias($1, $3); }
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
  | NIL                                                             { $$ = parse_nil_literal(); }
  | CURLY_L nl struct_literal_field_list nl CURLY_R                 { $$ = parse_struct_literal($3); }
  | SQUARE_L nl expression_list nl SQUARE_R                         { $$ = parse_list_literal($3); }
  | SQUARE_L nl map_entry_list nl SQUARE_R                          { $$ = parse_map_literal($3); }
  // | type DOT IDENTIFIER                                             { $$ = parse_enum_literal($3, $1); }
  // | DOT IDENTIFIER                                                  { $$ = parse_enum_literal($3, $1); }
  ;

variable: IDENTIFIER                                                { $$ = parse_named_variable($1); }
  | variable DOT IDENTIFIER                                         { $$ = parse_struct_member_variable($1, $3); }
  | variable SQUARE_L expression SQUARE_R                           { $$ = parse_indexed_variable($1, $3); }
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

group: PAREN_L expression PAREN_R                                   { $$ = parse_group($2); }
  ;

assignment: variable EQUAL expression                               { $$ = parse_assignment($1, $3); }
  ;

if_expr: IF expression block ELSE block                             { $$ = parse_if($2, $3, $5); }
  | IF expression block                                             { $$ = parse_if($2, $3, NULL); }
  | IF expression block ELSE if_expr                                { $$ = parse_nested_if($2, $3, $5); }
  ;

for_expr: FOR IDENTIFIER COMMA IDENTIFIER IN expression block       { $$ = parse_for($2, $4, $6, $7); }
  | FOR IDENTIFIER IN expression block                              { $$ = parse_for($2, NULL, $4, $5); }
  ;

block: CURLY_L nl statement_list nl CURLY_R                         { $$ = parse_block($3); }
  ;

type: IDENTIFIER                                                    { $$ = parse_named_type($1); }
  | type SQUARE_L SQUARE_R                                          { $$ = parse_list_type($1); }
  | type SQUARE_L type SQUARE_R                                     { $$ = parse_map_type($3, $1); }
  | SQUARE_L nl type_list nl SQUARE_R                               { $$ = parse_tuple_type($3); }
  | STRUCT CURLY_L nl struct_field_list nl CURLY_R                  { $$ = parse_struct_type($4); }
  | UNION CURLY_L nl type_list nl CURLY_R                           { $$ = parse_union_type($4); }
  | ENUM CURLY_L nl identifier_list nl CURLY_R                      { $$ = parse_enum_type($4); }
  | type QUESTION_MARK                                              { $$ = parse_optional_type($1); }
  | NIL                                                             { $$ = parse_nil_type(); }
  ;

struct_field_list: struct_field                                     { $$ = parse_struct_field_list($1, NULL); }
  | struct_field_list COMMA nl struct_field                         { $$ = parse_struct_field_list($4, $1); }
  ;

struct_field: IDENTIFIER COLON type                                 { $$ = parse_struct_field($1, $3, NULL); }
  | IDENTIFIER COLON type EQUAL expression                          { $$ = parse_struct_field($1, $3, $5); }
  ;

map_entry_list: map_entry                                           { $$ = parse_map_entry_list($1, NULL); }
  | map_entry_list COMMA nl map_entry                               { $$ = parse_map_entry_list($4, $1); }
  ;

map_entry: literal EQUAL expression                                 { $$ = parse_map_entry_literal($1, $3); }
  | block EQUAL expression                                          { $$ = parse_map_entry_block($1, $3); }
  ;

struct_literal_field_list: struct_literal_field                     { $$ = parse_struct_literal_field_list($1, NULL); }
  | struct_literal_field_list COMMA nl struct_literal_field         { $$ = parse_struct_literal_field_list($4, $1); }
  ;

struct_literal_field: DOT IDENTIFIER EQUAL expression               { $$ = parse_struct_literal_field($2, $4); }
  ;

type_list: type                                                     { $$ = parse_type_list($1, NULL); }
  | type_list COMMA nl type                                         { $$ = parse_type_list($4, $1); }
  ;

identifier_list: IDENTIFIER                                         { $$ = parse_identifier_list($1, NULL); }
  | identifier_list COMMA nl IDENTIFIER                             { $$ = parse_identifier_list($4, $1); }
  ;

expression_list: expression                                         { $$ = parse_expr_list($1, NULL); }
  | expression_list COMMA nl expression                             { $$ = parse_expr_list($4, $1); }
  ;

nl: NL                                                              {}
  | %empty                                                          {}
  ;

%%

static int yyreport_syntax_error(const yypcontext_t *ctx) {
  enum {MAX_EXPECTED = 10};
  yysymbol_kind_t expected[MAX_EXPECTED];
  int tokens_count = yypcontext_expected_tokens(ctx, expected, MAX_EXPECTED);
  if (tokens_count < 0) return tokens_count;
  const bool too_many_expected_tokens = tokens_count == 0 && expected[0] != YYSYMBOL_YYEMPTY;
  if (too_many_expected_tokens) tokens_count = MAX_EXPECTED;

  yysymbol_kind_t found = yypcontext_token(ctx);

  const YYLTYPE *loc = yypcontext_location(ctx);
  const char *expected_names[MAX_EXPECTED];
  for (u32 i = 0; i < tokens_count; i++) expected_names[i] = yysymbol_name(expected[i]);
  const char *found_name = found != YYSYMBOL_YYEMPTY ? yysymbol_name(found) : NULL;

  SyntaxErrorContext *ectx = new(SyntaxErrorContext);
  *ectx = (SyntaxErrorContext){
    .location = (Location *)loc,
    .found_token = found_name,
    .expected_tokens = expected_names,
    .expected_token_count = tokens_count,
  };

  fe_report_syntax_error(ectx);
}
