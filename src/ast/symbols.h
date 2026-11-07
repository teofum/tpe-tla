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
  const Value *value;
  Location loc;
  bool constant;
} VariableSymbol;

typedef enum {
  SYM_TYPE,
  SYM_FUNCTION,
  SYM_VAR,
} SymbolType;

typedef struct {
  SymbolType type;
  bool export;
  union {
    const TypeAliasStmt *type_alias;
    const FunctionDef *function;
    const VariableSymbol *var;
  };
} Symbol;

typedef map(SymbolKey, Symbol) SymbolTable;

Symbol *sym_type(const TypeAliasStmt *type_alias);
Symbol *sym_function(const FunctionDef *function);
Symbol *sym_var(const VariableSymbol *var);

const char *sym_get_type_str(const Symbol *sym);
Location sym_get_loc(const Symbol *sym);

usize sk_hash(const SymbolKey *key);
bool sk_compare(const SymbolKey *a, const SymbolKey *b);
SymbolKey *sk_clone(const SymbolKey *key);
void sk_free(SymbolKey *key);

#endif
