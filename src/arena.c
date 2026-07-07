#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "trident.h"

__arena_region__ *init_arena_region(size_t cap) {
  __arena_region__ *region = (__arena_region__ *)malloc(sizeof(__arena_region__));
  if (!region) {
    return NULL;
  }

  uint8_t *bytes = (uint8_t *)malloc(cap);
  if (!bytes) {
    free(region);
    return NULL;
  }

  *region = (__arena_region__){cap, 0, NULL, bytes};
  return region;
}

__arena__ *init_arena(size_t region_size) {
  __arena__ *arena = (__arena__ *)malloc(sizeof(__arena__));
  if (!arena) {
    return NULL;
  }

  *arena = (__arena__){region_size, NULL, NULL};
  return arena;
}

int push_new_arena_region(__arena__ *arena, size_t min_cap) {
  if (!arena) {
    return 0;
  }

  size_t cap = arena->region_size;
  if (cap < min_cap) {
    cap = min_cap;
  }

  __arena_region__ *region = init_arena_region(cap);
  if (!region) {
    return 0;
  }

  if (!arena->end) {
    arena->begin = region;
    arena->end = region;
  } else {
    arena->end->next = region;
    arena->end = region;
  }

  return 1;
}

void *arena_alloc(__arena__ *arena, size_t size) {
  if (!arena || size == 0) {
    return NULL;
  }

  if (!arena->end) {
    if (!push_new_arena_region(arena, size)) {
      return NULL;
    }
  }

  while (arena->end->offset + size > arena->end->cap && arena->end->next != NULL) {
    arena->end = arena->end->next;
  }

  if (arena->end->offset + size > arena->end->cap) {
    if (!push_new_arena_region(arena, size)) {
      return NULL;
    }
  }

  void *dest = arena->end->bytes + arena->end->offset;
  arena->end->offset += size;
  return dest;
}

void *arena_memdup(__arena__ *arena, void *data, size_t size) {
  if (!data || size == 0) {
    return NULL;
  }

  void *dest = arena_alloc(arena, size);
  if (!dest) {
    return NULL;
  }

  uint8_t *d = (uint8_t *)dest;
  uint8_t *s = (uint8_t *)data;
  for (size_t i = 0; i < size; ++i) {
    d[i] = s[i];
  }

  return dest;
}

void arena_reset(__arena__ *arena) {
  if (!arena) {
    return;
  }

  for (__arena_region__ *region = arena->begin; region != NULL; region = region->next) {
    region->offset = 0;
  }

  arena->end = arena->begin;
}

void free_arena(__arena__ *arena) {
  if (!arena) {
    return;
  }

  __arena_region__ *region = arena->begin;
  while (region) {
    __arena_region__ *next = region->next;
    free(region->bytes);
    free(region);
    region = next;
  }

  free(arena);
}
