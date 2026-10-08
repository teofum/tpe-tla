#ifndef AST_SYMBOLS_HEADER
#define AST_SYMBOLS_HEADER

#include <ast/ast.h>
#include <support/hash_map.h>
#include <support/types.h>

typedef struct {
  char *name;
  u32 scope;
} SymbolKey;

typedef struct {
  Type *type;
  // TODO value
} Symbol;

typedef map(SymbolKey, TypeAliasStmt) TypeTable;
typedef map(SymbolKey, FunctionDef) FunctionTable;
typedef map(SymbolKey, Symbol) SymbolTable;

typedef struct {
  TypeTable *types;
  FunctionTable *functions;
  SymbolTable *symbols;
} Symbols;

usize sk_hash(const SymbolKey *key);
bool sk_compare(const SymbolKey *a, const SymbolKey *b);
SymbolKey *sk_clone(const SymbolKey *key);
void sk_free(SymbolKey *key);

#endif
