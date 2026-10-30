#include <assert.h>
#include <stdlib.h>

#include <string.h>
#include <support/mem.h>
#include <support/types.h>
#include <support/util.h>

#include "dyn_array.h"

#define DEFAULT_CAP 4
#define next_cap(x) ((x) * 2)

void _da_base_init(_da_base *da, usize el_size, calloc_t alloc, free_t dealloc) {
  *da = (_da_base) {
    .el_size = el_size,
    .size = 0,
    .cap = DEFAULT_CAP,
    .alloc = alloc,
    .dealloc = dealloc,
    .destructor = NULL,
    .data = alloc(DEFAULT_CAP, el_size),
  };
}

void _da_base_manage(_da_base *da, _da_base_destructor d) {
  da->destructor = d;
}

void _da_base_free(_da_base *da) {
  _da_base_clear(da);
  if (da->dealloc) da->dealloc(da->data);
}

void _da_base_push(_da_base *da, void *v) {
  if (da->size == da->cap) _da_base_reserve(da, next_cap(da->cap));

  usize offset = da->size * da->el_size;
  memcpy(da->data + offset, v, da->el_size);
  da->size++;
}

void *_da_base_get(_da_base *da, usize i) {
  if (i >= da->size) return NULL;

  usize offset = i * da->el_size;
  return da->data + offset;
}

void *_da_base_next(_da_base *da, void *v) {
  if ((usize)(v - da->data) % da->el_size != 0) return NULL;
  usize i = (usize)(v - da->data) / da->el_size + 1;
  if (i >= da->size) return NULL;

  usize offset = i * da->el_size;
  return da->data + offset;
}

void _da_base_remove(_da_base *da, usize i) {
  if (i >= da->size) return;

  usize offset = i * da->el_size;
  if (da->destructor) da->destructor(da->data + offset);
  for (usize j = i + 1; j < da->size; j++) {
    usize j_offset = j * da->el_size;
    usize prev_offset = (j - 1) * da->el_size;
    memcpy(da->data + j_offset, da->data + prev_offset, da->el_size);
  }
  da->size--;
}

void _da_base_remove_unordered(_da_base *da, usize i) {
  if (i >= da->size) return;

  usize offset = i * da->el_size;
  usize last_offset = (da->size - 1) * da->el_size;
  if (da->destructor) da->destructor(da->data + offset);
  memcpy(da->data + offset, da->data + last_offset, da->el_size);
  da->size--;
}

void _da_base_clear(_da_base *da) {
  if (da->destructor) {
    for (usize i = 0; i < da->size; i++) {
      usize offset = i * da->el_size;
      da->destructor(da->data + offset);
    }
  }

  da->size = 0;
}

void _da_base_reserve(_da_base *da, usize cap) {
  if (cap <= da->cap) return;

  void *new_data = da->alloc(cap, da->el_size);
  assert(new_data != NULL);
  memcpy(new_data, da->data, da->el_size * da->size);
  if (da->dealloc) da->dealloc(da->data);
  da->data = new_data;
  da->cap = cap;
}

usize _da_base_size(_da_base *da) {
  return da->size;
}

usize _da_base_cap(_da_base *da) {
  return da->cap;
}
