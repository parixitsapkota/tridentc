#ifndef _TRIDENT_H_
#define _TRIDENT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// Reads the given file and returns it as a `char *`.
/// also sets bytes to number of bytes red if not `NULL`.
char *read_file(FILE *file, size_t *bytes);

// Hash set
/// Table grows (doubles bucket_cap) once count / bucket_cap exceeds this.
#define SHI_HS_LOAD_FACTOR 0.75 // 75 %
typedef struct __hs_entry__ {
  char *key;                 /* : Null-terminated key string (owned copy) */
  void *value;               /* : Stored value */
  int in_use;                /* : 0 if slot is empty, 1 if occupied */
  struct __hs_entry__ *next; /* : Next entry in this bucket's chain */
} __hs_entry__;

typedef struct __hash_set__ {
  size_t bucket_cap;      /* : Number of buckets in the table */
  size_t count;           /* : Number of key/value pairs stored */
  __hs_entry__ **buckets; /* : Array of bucket chain heads */
} __hash_set__;

#define Hs __hash_set__

/// Initializes a new __hash_set__ with `bucket_cap' buckets.
__hash_set__ *init_hash_set(size_t bucket_cap);

/// Hashes a null-terminated string (FNV-1a).
size_t hash_string(const char *str);

/// Inserts or updates the value associated with `key'.
void put_to_hash_set(__hash_set__ *set, const char *key, void *value);

/// Grows the set to `new_bucket_cap' buckets and rehashes every entry.
/// Called automatically by put_to_hash_set when the load factor is exceeded,
/// but can also be called directly to pre-size a set.
void resize_hash_set(__hash_set__ *set, size_t new_bucket_cap);

/// Returns the value associated with `key', or NULL if not found.
void *get_from_hash_set(__hash_set__ *set, const char *key);

/// Returns 1 if `key' exists in the set, 0 otherwise.
int has_in_hash_set(__hash_set__ *set, const char *key);

/// Removes `key' from the set. Returns 1 if it was present, 0 otherwise.
int del_from_hash_set(__hash_set__ *set, const char *key);

/// Frees the entire hash set, including all chained entries and their keys.
void free_hash_set(__hash_set__ *set);

// Arena
typedef struct __arena_region__ {
  size_t cap;                    /* : Region capacity in bytes */
  size_t offset;                 /* : Current bytes used in region */
  struct __arena_region__ *next; /* : Next region in chain */
  uint8_t *bytes;                /* : Actual memory region bytes */
} __arena_region__;

typedef struct __arena__ {
  size_t region_size; /* : Default size in bytes for new regions */
  __arena_region__ *begin;
  __arena_region__ *end;
} __arena__;

#define Arena __arena__

/// Initializes a new __arena_region__ with the given byte capacity.
__arena_region__ *init_arena_region(size_t cap);

/// Initializes a new __arena__ with the given default region size.
__arena__ *init_arena(size_t region_size);

/// Allocates a new region, chains it after the arena's current end, and
/// advances the arena's end pointer to it. Capacity is at least
/// `min_cap` bytes, or the arena's default region_size, whichever is larger.
int push_new_arena_region(__arena__ *arena, size_t min_cap);

/// Allocates `size` bytes from the arena. Walks forward through already
/// chained regions before allocating a new one.
void *arena_alloc(__arena__ *arena, size_t size);

/// Copies `size` bytes from `data` into a fresh allocation in the arena.
void *arena_memdup(__arena__ *arena, void *data, size_t size);

/// Resets all regions' offsets to 0 without freeing them, so the arena's
/// memory chain is fully reusable.
void arena_reset(__arena__ *arena);

/// Frees the entire region chain and the arena itself.
void free_arena(__arena__ *arena);

#endif // _TRIDENT_H_
