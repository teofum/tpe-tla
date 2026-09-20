#ifndef AST_HEADER
#define AST_HEADER

#include <support/types.h>

// -----------------------------------------------------------------------------

typedef enum {
  EXPR_LITERAL,
  EXPR_VARIABLE,
  EXPR_GROUP,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_ASSIGNMENT,
  EXPR_DECLARATION,
  EXPR_IF,
  EXPR_FOR,
  EXPR_BLOCK,
} ExprType;

typedef enum {
  L_INTEGER,
  L_FLOAT,
  L_STRING,
  L_BOOL,
} LiteralType;

typedef enum {
  V_IDENTIFIER,
  V_STRUCT_MEMBER,
} VariableType;

typedef struct {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} Location;

typedef struct {
  TokenLabel label;
  char *lexeme;
  u32 len;

  Location location;
} TokenMeta;

typedef struct Expr Expr;
typedef struct LiteralExpr LiteralExpr;
typedef struct VariableExpr VariableExpr;
typedef struct GroupExpr GroupExpr;
typedef struct UnaryExpr UnaryExpr;
typedef struct BinaryExpr BinaryExpr;
typedef struct AssignmentExpr AssignmentExpr;
typedef struct DeclarationExpr DeclarationExpr;
typedef struct IfExpr IfExpr;
typedef struct ForExpr ForExpr;
typedef struct BlockExpr BlockExpr;

typedef struct IntegerLiteral IntegerLiteral;
typedef struct FloatLiteral FloatLiteral;
typedef struct StringLiteral StringLiteral;
typedef struct BooleanLiteral BooleanLiteral;

typedef struct IdentifierVariable IdentifierVariable;
typedef struct StructMemberVariable StructMemberVariable;

typedef struct Program Program;
typedef struct ExprList ExprList;

typedef struct {
  Program *ast;
} CompilerState;

// -----------------------------------------------------------------------------

struct Expr {
  ExprType type;
  union {
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
  };
};

struct LiteralExpr {
  LiteralType type;
  union {
    IntegerLiteral *integer;
    FloatLiteral *floating;
    StringLiteral *string;
    BooleanLiteral *boolean;
  };
};

struct VariableExpr {
  VariableType type;
  union {
    IdentifierVariable *identifier;
    StructMemberVariable *struct_member;
  };
};

struct GroupExpr {
  Expr *inner_expr;
};

struct UnaryExpr {
  TokenMeta *op;
  Expr *expr;
};

struct BinaryExpr {
  TokenMeta *op;
  Expr *left;
  Expr *right;
};

struct AssignmentExpr {
  VariableExpr *left;
  Expr *right;
};

struct DeclarationExpr {
  IdentifierVariable *left;
  Expr *right;
  Expr *type; // TODO TypeExpr
};

struct IfExpr {
  Expr *condition;
  Expr *true_branch;
  Expr *false_branch;
};

struct ForExpr {
  const char *var_identifier;
  const char *idx_identifier;
  Expr *iterable;
  Expr *body;
};

struct BlockExpr {
  u32 len;
  Expr **exprs;
};

// -----------------------------------------------------------------------------

struct IntegerLiteral {
  i64 value;
  TokenMeta *meta;
};

struct FloatLiteral {
  f64 value;
  TokenMeta *meta;
};

struct StringLiteral {
  str value;
  TokenMeta *meta;
};

struct BooleanLiteral {
  bool value;
  TokenMeta *meta;
};

// -----------------------------------------------------------------------------

struct IdentifierVariable {
  TokenMeta *meta;
};

struct StructMemberVariable {
  Expr *struct_expr;
  TokenMeta *meta;
};

// -----------------------------------------------------------------------------

struct ExprList {
  u32 len;
  Expr *head;
  ExprList *tail;
};

struct Program {
  u32 len;
  Expr **exprs;
};

// -----------------------------------------------------------------------------

Expr *ast_expr_literal(LiteralExpr *literal);
Expr *ast_expr_variable(VariableExpr *var);
Expr *ast_expr_unary(UnaryExpr *unary);
Expr *ast_expr_binary(BinaryExpr *binary);
Expr *ast_expr_group(GroupExpr *group);
Expr *ast_expr_assignment(AssignmentExpr *assign);

LiteralExpr *ast_literal_integer(IntegerLiteral *i);
LiteralExpr *ast_literal_float(FloatLiteral *f);
LiteralExpr *ast_literal_string(StringLiteral *s);
LiteralExpr *ast_literal_boolean(BooleanLiteral *b);

VariableExpr *ast_variable_identifier(TokenMeta *id);
VariableExpr *ast_variable_struct_member(Expr *struct_expr, TokenMeta *id);

UnaryExpr *ast_unary(TokenMeta *op, Expr *expr);
BinaryExpr *ast_binary(Expr *left, TokenMeta *op, Expr *right);
GroupExpr *ast_group(Expr *expr);
AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right);

ExprList *ast_expr_list(Expr *head, ExprList *tail);
Program *ast_program(ExprList *exprs);

void ast_free_expr(Expr *expr);
void ast_free_literal(LiteralExpr *literal);
void ast_free_variable(VariableExpr *var);
void ast_free_unary(UnaryExpr *unary);
void ast_free_binary(BinaryExpr *binary);
void ast_free_group(GroupExpr *group);
void ast_free_assignment(AssignmentExpr *assign);

void ast_free_int_literal(IntegerLiteral *l);
void ast_free_float_literal(FloatLiteral *l);
void ast_free_string_literal(StringLiteral *l);
void ast_free_bool_literal(BooleanLiteral *l);

void ast_free_identifier_variable(IdentifierVariable *v);
void ast_free_struct_member_variable(StructMemberVariable *v);

void ast_free_meta(TokenMeta *meta);
void ast_free_expr_list(ExprList *list, bool free_exprs);
void ast_free_program(Program *program);

#endif
