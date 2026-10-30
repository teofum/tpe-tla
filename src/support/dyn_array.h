#ifndef DYN_ARRAY_HEADER
#define DYN_ARRAY_HEADER

#include <stdlib.h>

#include <support/mem.h>
#include <support/types.h>

typedef void (*_da_base_destructor)(void *);

typedef struct {
  usize el_size;
  usize size;
  usize cap;
  void *data;
  _da_base_destructor destructor;
  calloc_t alloc;
  free_t dealloc;
} _da_base;

#define dyn_array(T)              \
  struct {                        \
    _da_base _da;                 \
    struct {                      \
      T *el_t;                    \
      void (*destructor)(T *);    \
    } types[0];                   \
  }

#define EL_PTR(da) typeof((da)->types->el_t)
#define EL_TYPE(da) typeof(*(da)->types->el_t)

#define da_init(da)                                                       \
  _da_base_init(&(da)->_da, sizeof(EL_TYPE(da)), calloc, free)

#define da_inita(da, alloc, dealloc)                                      \
  _da_base_init(&(da)->_da, sizeof(EL_TYPE(da)), alloc, dealloc)

#define da_manage(da, el_destructor)                                      \
  do {                                                                    \
    typeof((da)->types->destructor) _destructor = (el_destructor);        \
    _da_base_manage(&(da)->_da, (_da_base_destructor)_destructor);        \
  } while(0)

#define da_free(da) _da_base_free(&(da)->_da)

#define da_push(da, el)                                                   \
  ({                                                                      \
    EL_PTR(da) _el = (el);                                                \
    _da_base_push(&(da)->_da, _el);                                       \
  })

#define da_get(da, i)                                                     \
  (EL_PTR(da)) _da_base_get(&(da)->_da, i)

#define da_next(da, el)                                                   \
  ({                                                                      \
    EL_PTR(da) _el = (el);                                                \
    (EL_PTR(da)) _da_base_next(&(da)->_da, _el);                          \
  })

#define da_remove(da, i)                                                  \
  _da_base_remove(&(da)->_da, i)

#define da_remove_unordered(da, i)                                        \
  _da_base_remove_unordered(&(da)->_da, i)

#define da_clear(da)                                                      \
  _da_base_clear(&(da)->_da)

#define da_reserve(da, cap)                                               \
  _da_base_reserve(&(da)->_da, cap)

#define da_size(da)                                                       \
  _da_base_size(&(da)->_da)

#define da_cap(da)                                                        \
  _da_base_cap(&(da)->_da)

#define da_pop(da, i)                                                     \
  da_remove_unordered(da, da_size(da) - 1)

#define da_peek(da, i)                                                    \
  da_get(da, da_size(da) - 1)

#define for_each(it_name, da) \
for (EL_PTR(da) it_name = da_get(da, 0); it_name != NULL; it_name = da_next(da, it_name))

void _da_base_init(_da_base *da, usize el_size, calloc_t alloc, free_t dealloc);
void _da_base_manage(_da_base *da, _da_base_destructor d);
void _da_base_free(_da_base *da);

void _da_base_push(_da_base *da, void *v);
void *_da_base_get(_da_base *da, usize i);
void *_da_base_next(_da_base *da, void *v);

void _da_base_remove(_da_base *da, usize i);
void _da_base_remove_unordered(_da_base *da, usize i);
void _da_base_clear(_da_base *da);

void _da_base_reserve(_da_base *da, usize cap);
usize _da_base_size(_da_base *da);
usize _da_base_cap(_da_base *da);

#endif
