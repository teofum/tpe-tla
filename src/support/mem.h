#ifndef MEM_HEADER
#define MEM_HEADER

#include <support/types.h>

#define MEM_SIZE_AUTO 0

typedef void *(*malloc_t)(usize);
typedef void *(*calloc_t)(usize, usize);
typedef void (*free_t)(void *);

typedef struct dyn_arena dyn_arena;

dyn_arena *mem_dyn_arena_create(usize block_size);
void mem_dyn_arena_destroy(dyn_arena *arena);

void *mem_dyn_arena_malloc(dyn_arena *arena, usize size);
void *mem_dyn_arena_calloc(dyn_arena *arena, usize count, usize size);

void mem_dyn_arena_free_all(dyn_arena *arena);

#endif
