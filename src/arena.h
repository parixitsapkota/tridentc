#ifndef _ARENA_H_
#define _ARENA_H_

/**
 * SHI_ARENA (Region Based Arena Allocator)
 *Original: https://github.com/oarixsapkota/SHI/blob/main/shi_arena.h
 *
 * IMPLEMENTATION:
 * Define SHI_ARENA_IMPLEMENTATION in exactly one C/C++ file before including
 * this header to instantiate the implementation block.
 *
 * #define SHI_ARENA_IMPLEMENTATION
 * #include "shi_arena.h"
 *
 * @note The arena is not thread-safe unless external synchronization is
 *       provided.
 *
 * @brief A single memory region belonging to an arena.
 *
 * An arena consists of one or more regions linked together as a singly
 * linked list. Each region owns a contiguous block of memory represented
 * by the flexible array member `data`.
 *
 * The `offset` and `cap` fields are measured in units of `uintptr_t`,
 * not bytes:
 *
 *     1 word = sizeof(uintptr_t) bytes
 *
 * `offset` represents the number of words already consumed in the region.
 * `cap` represents the total number of words available in the region.
 *
 * The usable memory range is:
 *
 *     data[0] ... data[cap - 1]
 *
 * and the next allocation begins at:
 *
 *     data[offset]
 *
 * @note Regions are owned by their parent arena and should not normally
 *       be created, modified, or freed independently by the caller.
 *
 *
 * @brief Memory arena consisting of a chain of reusable regions.
 *
 * The arena maintains three region pointers:
 *
 *     begin
 *       |
 *       v
 *     region -> region -> region -> NULL
 *                              ^
 *                              |
 *                             end
 *
 * `current` points to the region currently being used for allocations.
 *
 * When the current region does not have enough remaining space, the
 * allocator advances through existing regions. If none of the existing
 * regions can satisfy the allocation, a new region is allocated and
 * appended to the end of the chain.
 *
 * The arena does not individually free allocations. Memory remains owned
 * by the arena until the arena itself is destroyed with `free_arena()`.
 *
 * Calling `arena_reset()` makes all existing regions reusable without
 * returning their memory to the operating system.
 */

#include <stddef.h>
#include <stdint.h>

typedef struct __arena_region__ __arena_region__;

struct __arena_region__ {
  __arena_region__ *next; /// Next region in the arena's region chain.
  size_t offset;          /// Number of uintptr_t words currently used in this region.
  size_t cap;             /// Total capacity of this region in uintptr_t words.
  uintptr_t data[];       /// Flexible array containing the region's storage.
};

typedef struct {
  size_t region_size;        /// Default region capacity in uintptr_t words.
  __arena_region__ *begin;   /// First region in the arena's region chain.
  __arena_region__ *current; /// Region currently receiving allocations.
  __arena_region__ *end;     /// Last region in the arena's region chain.
} __arena__;

/**
 * @brief Public alias for __arena__.
 */
#define Arena __arena__

/**
 * @brief Initializes a new arena region.
 *
 * Allocates a single region capable of storing at least `cap`
 * uintptr_t words.
 *
 * The returned region starts empty:
 *
 *     region->offset == 0
 *
 * and has no successor:
 *
 *     region->next == NULL
 *
 * @param cap
 *     Number of uintptr_t words to reserve for the region payload.
 *
 * @return
 *     A pointer to the newly allocated region on success.
 *     NULL if the requested allocation would overflow or if memory
 *     allocation fails.
 *
 * @note `cap` is measured in uintptr_t words, not bytes.
 *
 * @note The returned region is not automatically attached to an arena.
 *       Use `push_new_arena_region()` when a region needs to become
 *       part of an arena's region chain.
 */
__arena_region__ *init_arena_region(size_t cap);

/**
 * @brief Initializes a new memory arena.
 *
 * Creates an empty arena with no allocated regions.
 *
 * The supplied `region_size` determines the default size of regions
 * allocated by the arena.
 *
 * If `region_size` is zero, the implementation uses
 * `ARENA_REGION_DEFAULT_CAPACITY` as the default region capacity.
 *
 * The supplied size is specified in bytes and is internally rounded
 * up to the nearest number of uintptr_t words.
 *
 * @param region_size
 *     Default region size in bytes.
 *     A value of zero selects the implementation's default capacity.
 *
 * @return
 *     A pointer to a newly initialized arena on success.
 *     NULL if the arena cannot be allocated or the requested size
 *     cannot be represented safely.
 *
 * @note No memory regions are allocated by this function itself.
 *       Regions are allocated lazily when `arena_alloc()` is first called.
 *
 * @note The returned arena must eventually be released with
 *       `free_arena()`.
 */
__arena__ *init_arena(size_t region_size);

/**
 * @brief Appends a new region to an arena.
 *
 * Allocates a new region and appends it to the end of the arena's
 * region chain.
 *
 * The new region's capacity is the larger of:
 *
 *     arena->region_size
 *     min_cap
 *
 * This allows large individual allocations to receive a region large
 * enough to hold the requested allocation instead of repeatedly
 * allocating smaller regions.
 *
 * After successful insertion:
 *
 *     arena->end     == new region
 *     arena->current == new region
 *
 * If the arena has no regions, the new region becomes `begin`,
 * `current`, and `end`.
 *
 * @param arena
 *     Arena to which the new region will be appended.
 *
 * @param min_cap
 *     Minimum capacity of the new region in uintptr_t words.
 *
 * @return
 *     Non-zero on success.
 *     Zero if `arena` is NULL or the new region cannot be allocated.
 *
 * @note The caller normally does not need to call this function directly.
 *       `arena_alloc()` automatically creates regions when required.
 */
int push_new_arena_region(__arena__ *arena, size_t min_cap);

/**
 * @brief Allocates memory from an arena.
 *
 * Reserves at least `size` bytes from the arena and returns a pointer to
 * the beginning of the allocated memory.
 *
 * The requested byte count is rounded up to the nearest multiple of
 * `sizeof(uintptr_t)` so that subsequent allocations remain correctly
 * aligned for the arena's storage unit.
 *
 * Allocation proceeds as follows:
 *
 * 1. If the arena has no regions, a new region is created.
 *
 * 2. The current region is checked for sufficient remaining capacity.
 *
 * 3. If necessary, the allocator advances through already allocated
 *    regions looking for one with enough free space.
 *
 * 4. If no existing region can satisfy the allocation, a new region
 *    is appended with enough capacity for the request.
 *
 * 5. The requested space is reserved and the region's offset is advanced.
 *
 * The arena does not support individual deallocation. The returned
 * memory remains valid until the arena is reset or freed.
 *
 * @param arena
 *     Arena from which to allocate memory.
 *
 * @param size
 *     Number of bytes to allocate.
 *
 * @return
 *     Pointer to the allocated memory on success.
 *     NULL if `arena` is NULL, `size` is zero, the requested size
 *     cannot be represented safely, or memory allocation fails.
 *
 * @warning Memory returned by this function becomes invalid for reuse
 *          after `arena_reset()` and invalid after `free_arena()`.
 *
 * @note The returned memory is aligned according to the arena's
 *       uintptr_t-based storage.
 *
 * @note Individual allocations cannot be freed.
 *
 */
void *arena_alloc(__arena__ *arena, size_t size);

/**
 * @brief Copies data into newly allocated arena memory.
 *
 * Allocates `size` bytes from the arena and copies the contents of
 * `data` into the newly allocated memory.
 *
 * This is equivalent to:
 *
 *     void *dest = arena_alloc(arena, size);
 *     memcpy(dest, data, size);
 *
 * except that the operation handles allocation failure internally.
 *
 * @param arena
 *     Arena into which the copy will be allocated.
 *
 * @param data
 *     Pointer to the source data.
 *
 * @param size
 *     Number of bytes to copy.
 *
 * @return
 *     Pointer to the copied data on success.
 *     NULL if `arena` is NULL, `data` is NULL, `size` is zero,
 *     or the allocation fails.
 *
 * @warning The source memory must contain at least `size` readable bytes.
 *
 * @warning The returned memory is owned by the arena and becomes
 *          reusable after `arena_reset()` and invalid after
 *          `free_arena()`.
 *
 * @note This function does not take ownership of `data`.
 *       The caller remains responsible for the source memory.
 */
void *arena_memdup(__arena__ *arena, const void *data, size_t size);

/**
 * @brief Resets an arena for reuse.
 *
 * Sets the allocation offset of every region in the arena back to zero.
 * No region is freed.
 *
 * After a reset, the complete region chain becomes available for
 * subsequent allocations:
 *
 *     begin
 *       |
 *       v
 *     region -> region -> region -> region
 *                                  ^
 *                                  |
 *                                 end
 *                                  ^
 *                                  |
 *                               current
 *
 * All regions retain their previously allocated capacity. Therefore,
 * resetting an arena does not return memory to the operating system.
 *
 * This behavior is intentional and is useful when an arena is repeatedly
 * used for temporary allocations, such as:
 *
 * - parsers
 * - compilers
 * - AST construction
 * - frame/scratch allocations
 * - request-local temporary data
 * - short-lived object graphs
 *
 * For example:
 *
 *     Arena *arena = init_arena(4096);
 *
 *     for (...) {
 *         build_temporary_data(arena);
 *         arena_reset(arena);
 *     }
 *
 * The arena can reuse memory obtained during previous iterations instead
 * of repeatedly requesting memory from the system allocator.
 *
 * @param arena
 *     Arena to reset. NULL is safely ignored.
 *
 * @warning All allocations previously returned by the arena should be
 *          considered invalid for continued use after this operation.
 *          The memory itself is not necessarily cleared; it is simply
 *          marked as available for reuse.
 *
 * @note Resetting an arena does not reduce its memory footprint.
 *       If minimizing retained memory is more important than allocation
 *       performance, use `free_arena()` or provide a separate trimming
 *       operation.
 */
void arena_reset(__arena__ *arena);

/**
 * @brief Frees an entire arena and all of its regions.
 *
 * Walks through the arena's region chain and releases every allocated
 * region, then releases the arena itself.
 *
 * After this function returns, the arena pointer and every pointer
 * previously returned by `arena_alloc()` or `arena_memdup()` for that
 * arena must no longer be used.
 *
 * The destruction order is:
 *
 *     region 1
 *         ↓
 *     region 2
 *         ↓
 *     region 3
 *         ↓
 *       arena
 *
 * Passing NULL is safe and has no effect.
 *
 * @param arena
 *     Arena to destroy.
 *
 * @warning This function invalidates all memory previously allocated
 *          from the arena.
 *
 * @note After calling this function, do not attempt to call
 *       `arena_alloc()`, `arena_reset()`, or any other arena operation
 *       using the same pointer.
 */
void free_arena(__arena__ *arena);

#endif // _ARENA_H_