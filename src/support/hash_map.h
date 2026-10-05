/*
 * Hashmap implementation largely based on https://github.com/DavidLeeds/hashmap
 * by David Leeds <davidesleeds@gmail.com>
 * rewritten from scratch because I wanted to know how hashmaps work
 */

#ifndef HASH_MAP_HEADER
#define HASH_MAP_HEADER

#include <support/types.h>

typedef usize (*_map_base_hash_fn)(const void *);
typedef bool (*_map_base_compare_fn)(const void *, const void *);
typedef void *(*_map_base_key_clone_fn)(const void *);
typedef void (*_map_base_destructor)(void *);

typedef struct map_entry map_entry;
typedef struct {
  usize size;
  usize table_size;
  map_entry *table;
  _map_base_hash_fn hash;
  _map_base_compare_fn compare;
  _map_base_key_clone_fn key_clone;
  _map_base_destructor key_destructor;
  _map_base_destructor value_destructor;
} _map_base;

#define map(K, V)                                           \
  struct {                                                  \
    _map_base _map;                                         \
    struct {                                                \
      const K *key;                                         \
      const V *value;                                       \
      usize (*hash_fn)(const K *);                          \
      bool (*compare_fn)(const K *, const K *);             \
      K *(*key_clone_fn)(const K *);                        \
      void (*key_destructor)(K *);                          \
      void (*value_destructor)(V *);                        \
    } map_types[0];                                         \
  }

#define map_init(m, hash_fn, compare_fn)                                        \
  do {                                                                          \
    typeof((m)->map_types.hash_fn) _hash_fn = (hash_fn);                        \
    typeof((m)->map_types.compare_fn) _compare_fn = (compare_fn);               \
    _map_base_init(&(m)->_map, (_map_base_hash_fn)_hash_fn,                     \
                               (_map_base_compare_fn)_compare_fn);              \
  } while(0)

#define map_manage_keys(m, key_clone_fn, key_destructor)                        \
  do {                                                                          \
    typeof((m)->map_types.key_clone_fn) _key_clone_fn = (key_clone_fn);         \
    typeof((m)->map_types.key_destructor) _key_destructor = (key_destructor);   \
    _map_base_manage_keys(&(m)->_map, (_map_base_key_clone_fn)_key_clone_fn,    \
                                      (_map_base_destructor)_key_destructor);   \
  } while(0)

#define map_manage_values(m, value_destructor)                                      \
  do {                                                                              \
    typeof((m)->map_types.value_destructor) _value_destructor = (value_destructor); \
    _map_base_manage_values(&(m)->_map, (_map_base_destructor)_value_destructor);   \
  } while(0)

#define map_insert(m, key, value)                                               \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    typeof((m)->map_types.value) _value = (value);                              \
    _map_base_insert(&(m)->_map, _key, _value);                                 \
  })

#define map_upsert(m, key, value)                                               \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    typeof((m)->map_types.value) _value = (value);                              \
    (typeof((m)->map_types.value) *)_map_base_upsert(&(m)->_map, _key, _value); \
  })

#define map_put(m, key, value)                                                  \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    typeof((m)->map_types.value) _value = (value);                              \
    _map_base_put(&(m)->_map, _key, _value);                                    \
  })

#define map_has(m, key)                                                         \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    _map_base_has(&(m)->_map, _key);                                            \
  })

#define map_get(m, key)                                                         \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    (typeof((m)->map_types.value) *)_map_base_get(&(m)->_map, _key);            \
  })

#define map_remove(m, key)                                                      \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    (typeof((m)->map_types.value) *)_map_base_remove(&(m)->_map, _key);         \
  })

#define map_delete(m, key)                                                      \
  ({                                                                            \
    typeof((m)->map_types.key) _key = (key);                                    \
    _map_base_delete(&(m)->_map, _key);                                         \
  })

#define map_free(m) _map_base_free(&((m)->_map))

void _map_base_init(_map_base *mb, _map_base_hash_fn hash_fn, _map_base_compare_fn compare_fn);
void _map_base_manage_keys(_map_base *mb, _map_base_key_clone_fn key_clone_fn, _map_base_destructor key_destructor);
void _map_base_manage_values(_map_base *mb, _map_base_destructor value_destructor);
void _map_base_free(_map_base *mb);

bool _map_base_insert(_map_base *mb, const void *key, void *value);
void *_map_base_upsert(_map_base *mb, const void *key, void *value);
bool _map_base_put(_map_base *mb, const void *key, void *value);

bool _map_base_has(const _map_base *mb, const void *key);
void *_map_base_get(const _map_base *mb, const void *key);

void *_map_base_remove(_map_base *mb, const void *key);
bool _map_base_delete(_map_base *mb, const void *key);

#endif
