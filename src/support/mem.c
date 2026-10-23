#include <stdlib.h>
#include <string.h>

#include <support/types.h>
#include <support/util.h>

#include "mem.h"

#define DEFAULT_BLOCK_SIZE 4096

typedef struct dyn_arena_block dyn_arena_block;
struct dyn_arena_block {
  void *data;
  void *available;
  dyn_arena_block *next;
};

struct dyn_arena {
  usize block_size;
  dyn_arena_block *first;
  dyn_arena_block *head;
};

// Align to 8-byte boundary
static usize _align(usize size) {
  return (size + 7) & -8ul;
}

static dyn_arena_block *_dyn_arena_make_block(usize size) {
  dyn_arena_block *block = new(dyn_arena_block);
  block->data = malloc(size);
  block->available = block->data;

  return block;
}

static void _dyn_arena_free_block(dyn_arena_block *block) {
  if (!block) return;

  block->available = block->data;
  _dyn_arena_free_block(block->next);
}

static void _dyn_arena_destroy_block(dyn_arena_block *block) {
  if (!block) return;

  _dyn_arena_destroy_block(block->next);
  free(block->data);
  free(block);
}

dyn_arena *mem_dyn_arena_create(usize block_size) {
  if (block_size == MEM_SIZE_AUTO) block_size = DEFAULT_BLOCK_SIZE;
  block_size = _align(block_size);

  dyn_arena *arena = new(dyn_arena);
  arena->block_size = block_size;
  arena->head = arena->first = _dyn_arena_make_block(block_size);

  return arena;
}

void mem_dyn_arena_destroy(dyn_arena *arena) {
  _dyn_arena_destroy_block(arena->first);
  free(arena);
}

void *mem_dyn_arena_malloc(dyn_arena *arena, usize size) {
  size = _align(size);

  usize bytes_used = arena->head->available - arena->head->data;
  usize bytes_available = arena->block_size - bytes_used;
  if (bytes_available < size) {
    if (!arena->head->next) {
      arena->head->next = _dyn_arena_make_block(arena->block_size);
    }
    arena->head = arena->head->next;
  }

  void *ptr = arena->head->available;
  arena->head->available += size;
  return ptr;
}

void *mem_dyn_arena_calloc(dyn_arena *arena, usize count, usize size) {
  void *ptr = mem_dyn_arena_malloc(arena, count * size);
  memset(ptr, 0, count * size);
  return ptr;
}

void mem_dyn_arena_free_all(dyn_arena *arena) {
  _dyn_arena_free_block(arena->first);
  arena->head = arena->first;
}
