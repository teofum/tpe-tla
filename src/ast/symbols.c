#include <string.h>

#include <support/hash_map.h>
#include <support/str.h>
#include <support/types.h>
#include <support/util.h>

#include "symbols.h"

Symbol *sym_type(const TypeAliasStmt *type_alias) {
  Symbol *sym = new(Symbol);
  *sym = (Symbol){ .type = SYM_TYPE, .type_alias = type_alias };
  return sym;
}

Symbol *sym_function(const FunctionDef *function) {
  Symbol *sym = new(Symbol);
  *sym = (Symbol){ .type = SYM_FUNCTION, .function = function };
  return sym;
}

Symbol *sym_var(const VariableSymbol *var) {
  Symbol *sym = new(Symbol);
  *sym = (Symbol){ .type = SYM_VAR, .var = var };
  return sym;
}

const char *sym_get_type_str(const Symbol *sym) {
  switch (sym->type) {
    case SYM_TYPE: return "type";
    case SYM_FUNCTION: return sym->function->body ? "function" : "builtin function";
    case SYM_VAR: return "variable";
  }
}

Location sym_get_loc(const Symbol *sym) {
  switch (sym->type) {
    case SYM_TYPE: return sym->type_alias->meta.loc;
    case SYM_FUNCTION: return sym->function->meta.loc;
    case SYM_VAR: return sym->var->loc;
  }
}

usize sk_hash(const SymbolKey *key) {
  usize hash = hash_cstring(key->name);
  hash = hash_bytes_append(hash, &key->scope, sizeof(key->scope));
  return hash;
}

bool sk_compare(const SymbolKey *a, const SymbolKey *b) {
  return a->scope == b->scope && strcmp(a->name, b->name) == 0;
}

SymbolKey *sk_clone(const SymbolKey *key) {
  SymbolKey *clone = new(SymbolKey);
  *clone = (SymbolKey){ .name = strdup(key->name), .scope = key->scope };
  return clone;
}

void sk_free(SymbolKey *key) {
  free(key->name);
  free(key);
}
