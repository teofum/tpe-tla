#ifndef AST_HEADER
#define AST_HEADER

#include <support/types.h>

// -----------------------------------------------------------------------------

typedef enum {
  STMT_EXPR,
  STMT_DECLARATION,
} StmtType;

typedef enum {
  EXPR_LITERAL,
  EXPR_VARIABLE,
  EXPR_GROUP,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_ASSIGNMENT,
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
  V_NAMED,
  V_STRUCT_MEMBER,
  V_INDEX,
} VariableType;

typedef enum {
  T_NAMED,
  T_STRUCT,
  T_UNION,
  T_LIST,
  T_MAP,
} TypeType;

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

typedef struct Stmt Stmt;
typedef struct ExprStmt ExprStmt;
typedef struct DeclarationStmt DeclarationStmt;

typedef struct Expr Expr;
typedef struct LiteralExpr LiteralExpr;
typedef struct VariableExpr VariableExpr;
typedef struct GroupExpr GroupExpr;
typedef struct UnaryExpr UnaryExpr;
typedef struct BinaryExpr BinaryExpr;
typedef struct AssignmentExpr AssignmentExpr;
typedef struct IfExpr IfExpr;
typedef struct ForExpr ForExpr;
typedef struct BlockExpr BlockExpr;

typedef struct IntegerLiteral IntegerLiteral;
typedef struct FloatLiteral FloatLiteral;
typedef struct StringLiteral StringLiteral;
typedef struct BooleanLiteral BooleanLiteral;

typedef struct NamedVariable NamedVariable;
typedef struct StructMemberVariable StructMemberVariable;
typedef struct IndexedVariable IndexedVariable;

typedef struct Type Type;
typedef struct NamedType NamedType;

typedef struct Program Program;
typedef struct StmtList StmtList;

typedef struct {
  Program *ast;
} CompilerState;

// -----------------------------------------------------------------------------

struct Stmt {
  StmtType type;
  union {
    Expr *expr;
    DeclarationStmt *decl;
  };
};

struct DeclarationStmt {
  TokenMeta *left;
  Expr *right;
  Type *type;
};

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
    IfExpr *if_expr;
    ForExpr *for_expr;
    BlockExpr *block;
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
    NamedVariable *named;
    StructMemberVariable *struct_member;
    IndexedVariable *indexed;
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
  Stmt **statements;
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

struct NamedVariable {
  TokenMeta *meta;
};

struct StructMemberVariable {
  VariableExpr *struct_expr;
  TokenMeta *meta;
};

struct IndexedVariable {
  VariableExpr *container;
  Expr *index;
};

// -----------------------------------------------------------------------------

struct Type {
  TypeType type;
  union {
    NamedType *named;
  };
};

struct NamedType {
  TokenMeta *meta;
};

// -----------------------------------------------------------------------------

struct StmtList {
  u32 len;
  Stmt *head;
  StmtList *tail;
};

struct Program {
  u32 len;
  Stmt **statements;
};

// -----------------------------------------------------------------------------

Stmt *ast_stmt_expr(Expr *expr);
Stmt *ast_stmt_decl(DeclarationStmt *decl);

DeclarationStmt *ast_declaration(TokenMeta *id, Type *type, Expr *expr);

Expr *ast_expr_literal(LiteralExpr *literal);
Expr *ast_expr_variable(VariableExpr *var);
Expr *ast_expr_unary(UnaryExpr *unary);
Expr *ast_expr_binary(BinaryExpr *binary);
Expr *ast_expr_group(GroupExpr *group);
Expr *ast_expr_assignment(AssignmentExpr *assign);
Expr *ast_expr_if(IfExpr *if_expr);
Expr *ast_expr_block(BlockExpr *block);

LiteralExpr *ast_literal_integer(IntegerLiteral *i);
LiteralExpr *ast_literal_float(FloatLiteral *f);
LiteralExpr *ast_literal_string(StringLiteral *s);
LiteralExpr *ast_literal_boolean(BooleanLiteral *b);

VariableExpr *ast_variable_named(TokenMeta *id);
VariableExpr *ast_variable_struct_member(VariableExpr *struct_expr, TokenMeta *id);
VariableExpr *ast_variable_indexed(VariableExpr *container, Expr *index);

UnaryExpr *ast_unary(TokenMeta *op, Expr *expr);
BinaryExpr *ast_binary(Expr *left, TokenMeta *op, Expr *right);
GroupExpr *ast_group(Expr *expr);
AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right);
IfExpr *ast_if(Expr *condition, Expr *true_branch, Expr *false_branch);
BlockExpr *ast_block(StmtList *statements, Expr *final);

Type *ast_type_named(TokenMeta *id);

StmtList *ast_stmt_list(Stmt *head, StmtList *tail);
Program *ast_program(StmtList *statements);

// -----------------------------------------------------------------------------

void ast_free_stmt(Stmt *stmt);
void ast_free_decl(DeclarationStmt *decl);

void ast_free_expr(Expr *expr);
void ast_free_literal(LiteralExpr *literal);
void ast_free_variable(VariableExpr *var);
void ast_free_unary(UnaryExpr *unary);
void ast_free_binary(BinaryExpr *binary);
void ast_free_group(GroupExpr *group);
void ast_free_assignment(AssignmentExpr *assign);
void ast_free_if(IfExpr *if_expr);
void ast_free_block(BlockExpr *block);

void ast_free_int_literal(IntegerLiteral *l);
void ast_free_float_literal(FloatLiteral *l);
void ast_free_string_literal(StringLiteral *l);
void ast_free_bool_literal(BooleanLiteral *l);

void ast_free_named_variable(NamedVariable *v);
void ast_free_struct_member_variable(StructMemberVariable *v);
void ast_free_indexed_variable(IndexedVariable *v);

void ast_free_type(Type *t);
void ast_free_named_type(NamedType *t);

void ast_free_meta(TokenMeta *meta);
void ast_free_stmt_list(StmtList *list, bool free_exprs);
void ast_free_program(Program *program);

#endif
