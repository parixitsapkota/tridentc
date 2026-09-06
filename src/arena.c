#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "trident.h"

#ifndef ARENA_REGION_DEFAULT_CAPACITY
#define ARENA_REGION_DEFAULT_CAPACITY (1024)
#endif

static int bytes_to_words(size_t bytes, size_t *out_words) {
  if (bytes > SIZE_MAX - (sizeof(uintptr_t) - 1)) {
    return 0;
  }
  *out_words = (bytes + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
  return 1;
}

__arena_region__ *init_arena_region(size_t capacity) {
  if (capacity > (SIZE_MAX - sizeof(__arena_region__)) / sizeof(uintptr_t)) {
    return NULL;
  }

  size_t size_bytes = sizeof(__arena_region__) + sizeof(uintptr_t) * capacity;
  __arena_region__ *region = (__arena_region__ *)malloc(size_bytes);
  if (!region) {
    return NULL;
  }

  region->next = NULL;
  region->offset = 0;
  region->cap = capacity;

  return region;
}

__arena__ *init_arena(size_t region_size_bytes) {
  __arena__ *arena = (__arena__ *)malloc(sizeof(__arena__));
  if (!arena) {
    return NULL;
  }

  size_t capacity_words = 0;
  if (region_size_bytes > 0) {
    if (!bytes_to_words(region_size_bytes, &capacity_words)) {
      free(arena);
      return NULL;
    }
  } else {
    capacity_words = ARENA_REGION_DEFAULT_CAPACITY;
  }

  *arena = (__arena__){
      .region_size = capacity_words,
      .begin = NULL,
      .current = NULL,
      .end = NULL,
  };

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
    arena->current = region;
    arena->end = region;
  } else {
    arena->end->next = region;
    arena->end = region;
    arena->current = region;
  }

  return 1;
}

void *arena_alloc(__arena__ *arena, size_t size_bytes) {
  if (!arena || size_bytes == 0) {
    return NULL;
  }

  size_t size_words = 0;
  if (!bytes_to_words(size_bytes, &size_words)) {
    return NULL;
  }

  if (!arena->current) {
    if (!push_new_arena_region(arena, size_words)) {
      return NULL;
    }
  }

  while (arena->current->offset > arena->current->cap ||
         size_words > arena->current->cap - arena->current->offset) {

    if (arena->current->offset > arena->current->cap) {
      return NULL;
    }

    if (arena->current->next == NULL) {
      if (!push_new_arena_region(arena, size_words)) {
        return NULL;
      }
      break;
    }

    arena->current = arena->current->next;
  }

  if (arena->current->cap - arena->current->offset < size_words) {
    if (!push_new_arena_region(arena, size_words)) {
      return NULL;
    }
  }

  void *dest = &arena->current->data[arena->current->offset];
  arena->current->offset += size_words;
  return dest;
}

void *arena_memdup(__arena__ *arena, const void *data, size_t size) {
  if (!arena || !data || size == 0) {
    return NULL;
  }

  void *dest = arena_alloc(arena, size);
  if (!dest) {
    return NULL;
  }

  memcpy(dest, data, size);
  return dest;
}

void arena_reset(__arena__ *arena) {
  if (!arena) {
    return;
  }

  for (__arena_region__ *region = arena->begin; region != NULL; region = region->next) {
    region->offset = 0;
  }

  arena->current = arena->begin;
}

void free_arena(__arena__ *arena) {
  if (!arena) {
    return;
  }

  __arena_region__ *region = arena->begin;
  while (region) {
    __arena_region__ *next = region->next;
    free(region);
    region = next;
  }

  free(arena);
}
