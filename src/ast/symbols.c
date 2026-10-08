#include <string.h>

#include <support/hash_map.h>
#include <support/util.h>

#include "symbols.h"

usize sk_hash(const SymbolKey *key) {
  return hash_djb2(key, sizeof(SymbolKey));
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
