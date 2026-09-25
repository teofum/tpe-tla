#include <assert.h>
#include <stdlib.h>

#include <string.h>
#include <support/types.h>
#include <support/util.h>

#include "dyn_array.h"

#define DEFAULT_CAP 4
#define next_cap(x) ((x) * 2)

struct dyn_array {
  usize el_size;
  usize size;
  usize cap;
  void *data;
  destructor_t el_destructor;
};

dyn_array *dyn_array_create(usize el_size, destructor_t destructor) {
  dyn_array *array = new(dyn_array);
  *array = (dyn_array){
    .size = 0,
    .cap = DEFAULT_CAP,
    .el_size = el_size,
    .data = calloc(DEFAULT_CAP, el_size),
    .el_destructor = destructor,
  };

  return array;
}

void dyn_array_free(dyn_array *da) {
  dyn_array_clear(da);
  free(da->data);
  free(da);
}

void dyn_array_push(dyn_array *da, void *v) {
  if (da->size == da->cap) dyn_array_reserve(da, next_cap(da->cap));

  usize offset = da->size * da->el_size;
  memcpy(da->data + offset, v, da->el_size);
  da->size++;
}

void *dyn_array_get(dyn_array *da, usize i) {
  if (i >= da->size) return NULL;

  usize offset = i * da->el_size;
  return da->data + offset;
}

void dyn_array_remove(dyn_array *da, usize i) {
  if (i >= da->size) return;

  usize offset = i * da->el_size;
  if (da->el_destructor) da->el_destructor(*((void**)(da->data + offset)));
  for (usize j = i + 1; j < da->size; j++) {
    usize j_offset = j * da->el_size;
    usize prev_offset = (j - 1) * da->el_size;
    memcpy(da->data + j_offset, da->data + prev_offset, da->el_size);
  }
  da->size--;
}

void dyn_array_remove_unordered(dyn_array *da, usize i) {
  if (i >= da->size) return;

  usize offset = i * da->el_size;
  usize last_offset = (da->size - 1) * da->el_size;
  if (da->el_destructor) da->el_destructor(*((void**)(da->data + offset)));
  memcpy(da->data + offset, da->data + last_offset, da->el_size);
  da->size--;
}

void dyn_array_clear(dyn_array *da) {
  if (da->el_destructor) {
    for (usize i = 0; i < da->size; i++) {
      usize offset = i * da->el_size;
      da->el_destructor(*((void**)(da->data + offset)));
    }
  }

  da->size = 0;
}

void dyn_array_reserve(dyn_array *da, usize cap) {
  if (cap <= da->cap) return;

  da->data = realloc(da->data, da->el_size * cap);
  assert(da->data != NULL);
}

usize dyn_array_size(dyn_array *da) {
  return da->size;
}

usize dyn_array_cap(dyn_array *da) {
  return da->cap;
}
