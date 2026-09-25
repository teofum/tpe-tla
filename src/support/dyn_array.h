#ifndef DYN_ARRAY_HEADER
#define DYN_ARRAY_HEADER

#include <support/types.h>

typedef void (*destructor_t)(void *);

typedef struct dyn_array dyn_array;

#define new_dyn_array(T) dyn_array_create(sizeof(T), NULL)
#define new_dyn_array_d(T, destructor) dyn_array_create(sizeof(T), (destructor_t)destructor)

dyn_array *dyn_array_create(usize el_size, destructor_t destructor);
void dyn_array_free(dyn_array *da);

void dyn_array_push(dyn_array *da, void *v);
void *dyn_array_get(dyn_array *da, usize i);

void dyn_array_remove(dyn_array *da, usize i);
void dyn_array_remove_unordered(dyn_array *da, usize i);
void dyn_array_clear(dyn_array *da);

void dyn_array_reserve(dyn_array *da, usize cap);
usize dyn_array_size(dyn_array *da);
usize dyn_array_cap(dyn_array *da);

#endif
