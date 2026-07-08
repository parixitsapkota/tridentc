#include <stdlib.h>
#include <string.h>

#include "trident.h"

// FNV-1a
size_t hash_string(const char *str) {
  size_t hash = (size_t)14695981039346656037ULL;
  while (*str) {
    hash ^= (unsigned char)(*str++);
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}

static char *__hs_strdup__(const char *str) {
  size_t len = strlen(str) + 1;
  char *copy = (char *)malloc(len);
  if (!copy) {
    return NULL;
  }
  for (size_t i = 0; i < len; ++i) {
    copy[i] = str[i];
  }
  return copy;
}

__hash_set__ *init_hash_set(size_t bucket_cap) {
  if (bucket_cap == 0) {
    bucket_cap = 1;
  }

  __hash_set__ *set = (__hash_set__ *)malloc(sizeof(__hash_set__));
  if (!set) {
    return NULL;
  }

  __hs_entry__ **buckets = (__hs_entry__ **)calloc(bucket_cap, sizeof(__hs_entry__ *));
  if (!buckets) {
    free(set);
    return NULL;
  }

  set->bucket_cap = bucket_cap;
  set->count = 0;
  set->buckets = buckets;
  return set;
}

void resize_hash_set(__hash_set__ *set, size_t new_bucket_cap) {
  if (!set || new_bucket_cap == 0 || new_bucket_cap == set->bucket_cap) {
    return;
  }

  __hs_entry__ **new_buckets = (__hs_entry__ **)calloc(new_bucket_cap, sizeof(__hs_entry__ *));
  if (!new_buckets) {
    return;
  }

  // entries are re-threaded in place.
  for (size_t i = 0; i < set->bucket_cap; ++i) {
    __hs_entry__ *current = set->buckets[i];
    while (current != NULL) {
      __hs_entry__ *next = current->next;
      size_t new_index = hash_string(current->key) % new_bucket_cap;

      current->next = new_buckets[new_index];
      new_buckets[new_index] = current;

      current = next;
    }
  }

  free(set->buckets);
  set->buckets = new_buckets;
  set->bucket_cap = new_bucket_cap;
}

void put_to_hash_set(__hash_set__ *set, const char *key, void *value) {
  if (!set || !key) {
    return;
  }

  size_t index = hash_string(key) % set->bucket_cap;
  __hs_entry__ *current = set->buckets[index];

  while (current != NULL) {
    if (current->in_use && strcmp(current->key, key) == 0) {
      current->value = value;
      return;
    }
    current = current->next;
  }

  __hs_entry__ *entry = (__hs_entry__ *)malloc(sizeof(__hs_entry__));
  if (!entry) {
    return;
  }

  entry->key = __hs_strdup__(key);
  entry->value = value;
  entry->in_use = 1;
  entry->next = set->buckets[index];

  set->buckets[index] = entry;
  ++set->count;

  // cross-multiplied to dodge float division on every insert
  if ((double)set->count > (double)set->bucket_cap * SHI_HS_LOAD_FACTOR) {
    resize_hash_set(set, set->bucket_cap * 2);
  }
}

void *get_from_hash_set(__hash_set__ *set, const char *key) {
  if (!set || !key) {
    return NULL;
  }

  size_t index = hash_string(key) % set->bucket_cap;
  __hs_entry__ *current = set->buckets[index];

  while (current != NULL) {
    if (current->in_use && strcmp(current->key, key) == 0) {
      return current->value;
    }
    current = current->next;
  }
  return NULL;
}

int has_in_hash_set(__hash_set__ *set, const char *key) {
  if (!set || !key) {
    return 0;
  }

  size_t index = hash_string(key) % set->bucket_cap;
  __hs_entry__ *current = set->buckets[index];

  while (current != NULL) {
    if (current->in_use && strcmp(current->key, key) == 0) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}

int del_from_hash_set(__hash_set__ *set, const char *key) {
  if (!set || !key) {
    return 0;
  }

  size_t index = hash_string(key) % set->bucket_cap;
  __hs_entry__ *current = set->buckets[index];
  __hs_entry__ *prev = NULL;

  while (current != NULL) {
    if (current->in_use && strcmp(current->key, key) == 0) {
      if (prev) {
        prev->next = current->next;
      } else {
        set->buckets[index] = current->next;
      }

      free(current->key);
      free(current);
      --set->count;
      return 1;
    }
    prev = current;
    current = current->next;
  }
  return 0;
}

void free_hash_set(__hash_set__ *set) {
  if (!set) {
    return;
  }

  for (size_t i = 0; i < set->bucket_cap; ++i) {
    __hs_entry__ *current = set->buckets[i];
    while (current != NULL) {
      __hs_entry__ *temp = current->next;
      free(current->key);
      free(current);
      current = temp;
    }
  }

  free(set->buckets);
  free(set);
}
