#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <support/types.h>
#include <support/util.h>

#include "hash_map.h"

#define mod(mb, idx) ((idx) & ((mb)->table_size - 1))
#define next(mb, idx) (((idx) + 1) & ((mb)->table_size - 1))

#define INITIAL_SIZE 128

struct map_entry {
  void *key;
  void *value;
};

void _map_base_init(_map_base *mb, _map_base_hash_fn hash_fn, _map_base_compare_fn compare_fn) {
  assert(hash_fn != NULL);
  assert(compare_fn != NULL);

  memset(mb, 0, sizeof(_map_base));
  *mb = (_map_base){
    .hash = hash_fn,
    .compare = compare_fn,
  };
}

void _map_base_manage_keys(_map_base *mb, _map_base_key_clone_fn key_clone_fn, _map_base_destructor key_destructor) {
  assert(mb->size == 0);
  mb->key_clone = key_clone_fn;
  mb->key_destructor = key_destructor;
}

void _map_base_manage_values(_map_base *mb, _map_base_destructor value_destructor) {
  assert(mb->size == 0);
  mb->value_destructor = value_destructor;
}

static usize _map_idx(const _map_base *mb, const void *key) {
  // Fast modulo operation, assumes table size is a power of two
  return mb->hash(key) & (mb->table_size - 1);
}

static map_entry *_map_find(const _map_base *mb, const void *key, bool find_empty) {
  usize idx = _map_idx(mb, key);

  for (usize i = 0; i < mb->table_size; i++) {
    map_entry *entry = &mb->table[idx];

    if (!entry->key) return find_empty ? entry : NULL;
    if (mb->compare(entry->key, key)) return entry;

    idx = next(mb, idx);
  }
  return NULL;
}

static void *_map_remove(_map_base *mb, map_entry *removed) {
  usize removed_idx = removed - mb->table;
  void *value = removed->value;

  if (mb->key_destructor) mb->key_destructor(removed->key);
  mb->size--;

  usize idx = next(mb, removed_idx);
  for (usize i = 0; i < mb->table_size; i++) {
    map_entry *entry = &mb->table[idx];
    if (!entry->key) break;

    usize entry_base_idx = _map_idx(mb, entry->key);
    if (mod(mb, idx - entry_base_idx) > mod(mb, removed_idx - entry_base_idx)) {
      *removed = *entry;
      removed_idx = idx;
      removed = entry;
    }
    idx = next(mb, idx);
  }
  memset(removed, 0, sizeof(map_entry));

  return value;
}

static void _map_rehash(_map_base *mb, usize new_size) {
  assert((new_size & (new_size - 1)) == 0);
  assert(new_size >= mb->size);

  map_entry *new_table = new_array(map_entry, new_size);
  assert(new_table);

  map_entry *old_table = mb->table;
  usize old_size = mb->table_size;
  mb->table = new_table;
  mb->table_size = new_size;

  if (old_table) {
    for (map_entry *entry = old_table; entry < old_table + old_size; entry++) {
      if (!entry->key) continue;

      map_entry *new_entry = _map_find(mb, entry->key, true);
      assert(new_entry);
      *new_entry = *entry;
    }
    free(old_table);
  }
}

static void _map_resize(_map_base *mb) {
  if (mb->table_size == 0) {
    _map_rehash(mb, INITIAL_SIZE);
  } else {
    f32 load_factor = (f32)mb->size / mb->table_size;
    if (load_factor > 0.75) {
      _map_rehash(mb, mb->table_size << 1);
    }
    if (load_factor < 0.25 && mb->table_size > INITIAL_SIZE) {
      _map_rehash(mb, mb->table_size >> 1);
    }
  }
}

static void _map_free_keys(_map_base *mb) {
  if (!mb || !mb->key_destructor || mb->size == 0) return;

  for (map_entry *e = mb->table; e < &mb->table[mb->table_size]; e++) {
    if (e->key) mb->key_destructor(e->key);
  }
}

static void _map_free_values(_map_base *mb) {
  if (!mb || !mb->value_destructor || mb->size == 0) return;

  for (map_entry *e = mb->table; e < &mb->table[mb->table_size]; e++) {
    if (e->value) mb->value_destructor(e->value);
  }
}

void _map_base_free(_map_base *mb) {
  if (!mb) return;

  _map_free_keys(mb);
  _map_free_values(mb);
  free(mb->table);
  memset(mb, 0, sizeof(_map_base));
}

// Insert a value. Fails if the key already exists.
bool _map_base_insert(_map_base *mb, const void *key, void *value) {
  if (!key || !value) return false;

  _map_resize(mb);
  map_entry *entry = _map_find(mb, key, true);
  assert(entry);

  if (entry->key) return false;

  if (mb->key_clone) {
    entry->key = mb->key_clone(key);
  } else {
    entry->key = (void *)key;
  }

  entry->value = value;
  mb->size++;
  return true;
}

// Insert or update a value. Returns the old value, or NULL if it was absent.
void *_map_base_upsert(_map_base *mb, const void *key, void *value) {
  if (!key || !value) return NULL;

  _map_resize(mb);
  map_entry *entry = _map_find(mb, key, true);
  assert(entry);

  if (!entry->key) {
    if (mb->key_clone) {
      entry->key = mb->key_clone(key);
    } else {
      entry->key = (void *)key;
    }
    mb->size++;
  }

  void *old_value = entry->value;
  entry->value = value;

  return old_value;
}

// Insert a value, deleting the old value if present. May leak if the map does not have a value destructor.
bool _map_base_put(_map_base *mb, const void *key, void *value) {
  if (!key || !value) return false;

  _map_resize(mb);
  map_entry *entry = _map_find(mb, key, true);
  assert(entry);

  if (!entry->key) {
    if (mb->key_clone) {
      entry->key = mb->key_clone(key);
    } else {
      entry->key = (void *)key;
    }
    mb->size++;
  }

  void *old_value = entry->value;
  entry->value = value;

  if (old_value && mb->value_destructor) {
    mb->value_destructor(old_value);
  }

  return true;
}

bool _map_base_has(const _map_base *mb, const void *key) {
  return _map_find(mb, key, false) != NULL;
}

void *_map_base_get(const _map_base *mb, const void *key) {
  map_entry *e = _map_find(mb, key, false);
  return e ? e->value : NULL;
}

// Remove a value from the map, returning the removed value. Returns NULL if absent.
void *_map_base_remove(_map_base *mb, const void *key) {
  if (!key) return NULL;

  map_entry *entry = _map_find(mb, key, false);
  if (!entry) return NULL;

  return _map_remove(mb, entry);
}

// Remove a value from the map and delete it. May leak if the map does not have a value destructor.
bool _map_base_delete(_map_base *mb, const void *key) {
  if (!key) return false;

  map_entry *entry = _map_find(mb, key, false);
  if (!entry) return false;

  void *value = _map_remove(mb, entry);
  if (mb->value_destructor) mb->value_destructor(value);
  return true;
}
