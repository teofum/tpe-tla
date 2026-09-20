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
    i64 int_value;
    f64 float_value;
    str string_value;
    bool bool_value;
  };
};

struct VariableExpr {
  const char *identifier;
};

struct GroupExpr {
  Expr *inner_expr;
};

struct UnaryExpr {
  TokenLabel op;
  Expr *expr;
};

struct BinaryExpr {
  TokenLabel op;
  Expr *left;
  Expr *right;
};

struct AssignmentExpr {
  Expr *left;
  Expr *right;
};

struct DeclarationExpr {
  Expr *left;
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
Expr *ast_expr_unary(UnaryExpr *unary);
Expr *ast_expr_binary(BinaryExpr *binary);

LiteralExpr *ast_literal_integer(i64 i);
LiteralExpr *ast_literal_float(f64 f);
LiteralExpr *ast_literal_string(str s);
LiteralExpr *ast_literal_boolean(bool b);

UnaryExpr *ast_unary(TokenLabel op, Expr *expr);
BinaryExpr *ast_binary(Expr *left, TokenLabel op, Expr *right);

ExprList *ast_expr_list(Expr *head, ExprList *tail);
Program *ast_program(ExprList *exprs);

void ast_free_expr(Expr *expr);
void ast_free_literal(LiteralExpr *literal);
void ast_free_unary(UnaryExpr *unary);
void ast_free_binary(BinaryExpr *binary);
void ast_free_expr_list(ExprList *list, bool free_exprs);
void ast_free_program(Program *program);

#endif
