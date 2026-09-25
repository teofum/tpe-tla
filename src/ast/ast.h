#ifndef AST_HEADER
#define AST_HEADER

#include <support/types.h>

#define AST_LIST(T, name)                                     \
typedef struct T##List T##List;                               \
struct T##List {                                              \
  u32 len;                                                    \
  T *head;                                                    \
  T##List *tail;                                              \
};                                                            \
T##List *ast_##name##_list(T *head, T##List *tail);           \
void ast_free_##name##_list(T##List *list, bool free_items);  \

// -----------------------------------------------------------------------------

typedef enum {
  STMT_EXPR,
  STMT_DECLARATION,
  STMT_TYPE_ALIAS,
  STMT_FUNCTION_DEF,
  STMT_PARSE_ERROR, // Shouldn't exist in any valid AST; only used for debug output
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
  L_NIL,
  L_LIST,
  L_MAP,
  L_STRUCT,
  L_ENUM,
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
  T_TUPLE,
  T_ENUM,
  T_NIL,
} TypeType;

typedef struct {
  i32 first_line;
  i32 first_column;
  i32 last_line;
  i32 last_column;
} Location;

typedef struct {
  TokenLabel label;
  char *lexeme;
  u32 len;
} TokenMeta;

typedef TokenMeta Identifier;

typedef struct Stmt Stmt;
typedef struct DeclarationStmt DeclarationStmt;
typedef struct TypeAliasStmt TypeAliasStmt;

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
typedef struct ListLiteral ListLiteral;
typedef struct MapLiteral MapLiteral;
typedef struct MapEntry MapEntry;
typedef struct StructLiteral StructLiteral;
typedef struct StructLiteralField StructLiteralField;
typedef struct EnumLiteral EnumLiteral;

typedef struct NamedVariable NamedVariable;
typedef struct StructMemberVariable StructMemberVariable;
typedef struct IndexedVariable IndexedVariable;

typedef struct Type Type;
typedef struct NamedType NamedType;
typedef struct ListType ListType;
typedef struct MapType MapType;
typedef struct StructField StructField;
typedef struct StructType StructType;
typedef struct UnionType UnionType;
typedef struct TupleType TupleType;
typedef struct EnumType EnumType;

typedef struct FunctionDef FunctionDef;
typedef struct Parameter Parameter;

typedef struct Program Program;

AST_LIST(Stmt, stmt);
AST_LIST(Expr, expr);
AST_LIST(StructField, struct_field);
AST_LIST(StructLiteralField, struct_literal_field);
AST_LIST(MapEntry, map_entry);
AST_LIST(Type, type);
AST_LIST(Identifier, identifier);
AST_LIST(Parameter, parameter);

// -----------------------------------------------------------------------------

typedef struct {
  Program *ast;
} CompilerState;

// -----------------------------------------------------------------------------

struct Stmt {
  StmtType type;
  union {
    Expr *expr;
    DeclarationStmt *decl;
    TypeAliasStmt *type_alias;
    FunctionDef *function_def;
  };
};

struct DeclarationStmt {
  Identifier *left;
  Expr *right;
  Type *type;
};

struct TypeAliasStmt {
  Identifier *alias;
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
    ListLiteral *list;
    MapLiteral *map;
    StructLiteral *struct_literal;
    EnumLiteral *enum_literal;
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
  TokenLabel op;
  Expr *expr;
};

struct BinaryExpr {
  TokenLabel op;
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
  Identifier *var_id;
  Identifier *idx_id;
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

struct ListLiteral {
  u32 len;
  Expr **exprs;
};

struct MapLiteral {
  u32 len;
  MapEntry **entries;
};

struct MapEntry {
  Expr *key;
  Expr *value;
};

struct StructLiteral {
  u32 len;
  StructLiteralField **fields;
};

struct StructLiteralField {
  Identifier *name;
  Expr *value;
};

struct EnumLiteral {
  Type *type;
  Identifier *value;
};

// -----------------------------------------------------------------------------

struct NamedVariable {
  Identifier *name;
};

struct StructMemberVariable {
  VariableExpr *struct_expr;
  Identifier *name;
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
    ListType *list;
    MapType *map;
    StructType *struct_type;
    UnionType *union_type;
    TupleType *tuple;
    EnumType *enum_type;
  };
};

struct NamedType {
  Identifier *name;
};

struct ListType {
  Type *item_type;
};

struct MapType {
  Type *key_type;
  Type *value_type;
};

struct StructType {
  u32 len;
  StructField **fields;
};

struct UnionType {
  u32 len;
  Type **types;
};

struct TupleType {
  u32 len;
  Type **types;
};

struct EnumType {
  u32 len;
  Identifier **values;
};

struct StructField {
  Identifier *name;
  Type *type;
  Expr *default_value;
};

// -----------------------------------------------------------------------------

struct FunctionDef {
  Identifier *name;
  u32 params_len;
  Parameter **params;
  Type *return_type;
  BlockExpr *body;
};

struct Parameter {
  Identifier *name;
  Type *type;
  Expr *default_value;
};

// -----------------------------------------------------------------------------

struct Program {
  u32 len;
  Stmt **statements;
};

// -----------------------------------------------------------------------------

extern const char *TOKEN_LABEL_STR[];
extern const char *TOKEN_LEXEME[];

Stmt *ast_stmt_expr(Expr *expr);
Stmt *ast_stmt_decl(DeclarationStmt *decl);
Stmt *ast_stmt_alias(TypeAliasStmt *alias);
Stmt *ast_stmt_function_def(FunctionDef *function);
Stmt *ast_stmt_error();

DeclarationStmt *ast_declaration(Identifier *id, Type *type, Expr *expr);
TypeAliasStmt *ast_type_alias(Identifier *id, Type *type);

Expr *ast_expr_literal(LiteralExpr *literal);
Expr *ast_expr_variable(VariableExpr *var);
Expr *ast_expr_unary(UnaryExpr *unary);
Expr *ast_expr_binary(BinaryExpr *binary);
Expr *ast_expr_group(GroupExpr *group);
Expr *ast_expr_assignment(AssignmentExpr *assign);
Expr *ast_expr_if(IfExpr *if_expr);
Expr *ast_expr_for(ForExpr *for_expr);
Expr *ast_expr_block(BlockExpr *block);

LiteralExpr *ast_integer_literal(IntegerLiteral *i);
LiteralExpr *ast_float_literal(FloatLiteral *f);
LiteralExpr *ast_string_literal(StringLiteral *s);
LiteralExpr *ast_boolean_literal(BooleanLiteral *b);
LiteralExpr *ast_list_literal(ExprList *exprs);
LiteralExpr *ast_map_literal(MapEntryList *entries);
LiteralExpr *ast_struct_literal(StructLiteralFieldList *fields);
LiteralExpr *ast_enum_literal(Type *type, Identifier *id);
LiteralExpr *ast_nil_literal();

VariableExpr *ast_variable_named(Identifier *id);
VariableExpr *ast_variable_struct_member(VariableExpr *struct_expr, Identifier *id);
VariableExpr *ast_variable_indexed(VariableExpr *container, Expr *index);

UnaryExpr *ast_unary(TokenLabel op, Expr *expr);
BinaryExpr *ast_binary(Expr *left, TokenLabel op, Expr *right);
GroupExpr *ast_group(Expr *expr);
AssignmentExpr *ast_assignment(VariableExpr *left, Expr *right);
IfExpr *ast_if(Expr *condition, Expr *true_branch, Expr *false_branch);
ForExpr *ast_for(Identifier *var, Identifier *idx, Expr *iterable, Expr *body);
BlockExpr *ast_block(StmtList *statements);

Type *ast_named_type(Identifier *id);
Type *ast_list_type(Type *item_type);
Type *ast_map_type(Type *key_type, Type *value_type);
Type *ast_struct_type(StructFieldList *fields);
Type *ast_union_type(TypeList *types);
Type *ast_tuple_type(TypeList *types);
Type *ast_enum_type(IdentifierList *values);
Type *ast_nil_type();

FunctionDef *ast_function_def(Identifier *id, ParameterList *params, Type *return_type, BlockExpr *body);

StructField *ast_struct_field(Identifier *id, Type *type, Expr *default_value);
StructLiteralField *ast_struct_literal_field(Identifier *id, Expr *value);
MapEntry *ast_map_entry(Expr *key, Expr *value);
Parameter *ast_parameter(Identifier *id, Type *type, Expr *default_value);

Program *ast_program(StmtList *statements);

// -----------------------------------------------------------------------------

void ast_free_stmt(Stmt *stmt);
void ast_free_decl(DeclarationStmt *decl);
void ast_free_alias(TypeAliasStmt *alias);

void ast_free_expr(Expr *expr);
void ast_free_literal(LiteralExpr *literal);
void ast_free_variable(VariableExpr *var);
void ast_free_unary(UnaryExpr *unary);
void ast_free_binary(BinaryExpr *binary);
void ast_free_group(GroupExpr *group);
void ast_free_assignment(AssignmentExpr *assign);
void ast_free_if(IfExpr *if_expr);
void ast_free_for(ForExpr *for_expr);
void ast_free_block(BlockExpr *block);

void ast_free_int_literal(IntegerLiteral *l);
void ast_free_float_literal(FloatLiteral *l);
void ast_free_string_literal(StringLiteral *l);
void ast_free_bool_literal(BooleanLiteral *l);
void ast_free_list_literal(ListLiteral *l);
void ast_free_map_literal(MapLiteral *l);
void ast_free_struct_literal(StructLiteral *l);
void ast_free_enum_literal(EnumLiteral *l);

void ast_free_named_variable(NamedVariable *v);
void ast_free_struct_member_variable(StructMemberVariable *v);
void ast_free_indexed_variable(IndexedVariable *v);

void ast_free_type(Type *t);
void ast_free_named_type(NamedType *t);
void ast_free_list_type(ListType *t);
void ast_free_map_type(MapType *t);
void ast_free_struct_type(StructType *t);
void ast_free_union_type(UnionType *t);
void ast_free_tuple_type(TupleType *t);
void ast_free_enum_type(EnumType *t);

void ast_free_function_def(FunctionDef *f);

void ast_free_struct_field(StructField *f);
void ast_free_struct_literal_field(StructLiteralField *f);
void ast_free_map_entry(MapEntry *e);
void ast_free_parameter(Parameter *p);

void ast_free_identifier(Identifier *id);
void ast_free_token(TokenMeta *meta);
void ast_free_program(Program *program);

#endif
