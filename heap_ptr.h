#ifndef HEAP_PTR_H
#define HEAP_PTR_H

/*
 * A pointer to the heap consists of 3 parts:
 * - Metadata: The first byte represents metadata about the heap value.
 *   - Currently this includes the "mark" portion of mark and sweep gc as the highest bit.
 *   - The second highest bit is set if the pointer points to an array.
 *   - The third highest bit is set if the pointer points to an array of objects.
 *   - TBD if the other bits will be used. Maybe I will use them to make the size portion larger.
 * - Count: The next 24 bytes store the size of the data. For most objects this will be small,
 *   but arrays could possibly use up the full range.
 * - Location: The last 32 bits store the location in the heap: that means our heap can store
 *   up to about 4 gigabytes of data before hitting this limit.
 */
#define COUNT_OFFSET         UINT64_C(32)
#define COUNT_MAX            (UINT64_C(1) * UINT16_MAX * 3 / 2)
#define METADATA_OFFSET      UINT64_C(56)
#define GC_MARK_OFFSET       UINT64_C(63)
#define ARRAY_BIT_OFFSET     UINT64_C(62)
#define ARRAY_OBJ_BIT_OFFSET UINT64_C(61)
#define METADATA_MASK        (UINT64_MAX - ((UINT64_C(1) << METADATA_OFFSET) - 1))
#define LOCATION_MASK        ((UINT64_C(1) << COUNT_OFFSET) - 1)
#define COUNT_MASK           (UINT64_MAX - (LOCATION_MASK + METADATA_MASK))
#define GC_MARK_MASK         (UINT64_C(1) << GC_MARK_OFFSET)
#define ARRAY_BIT_MASK       (UINT64_C(1) << ARRAY_BIT_OFFSET)
#define ARRAY_OBJ_BIT_MASK   (UINT64_C(1) << ARRAY_OBJ_BIT_OFFSET)

typedef uint64_t HeapPointer;

static inline void set_count_no_check(HeapPointer *ptr, size_t count)
{
    *ptr = *ptr | (count << COUNT_OFFSET);
}

static inline bool set_count(HeapPointer *ptr, size_t count)
{
    if (count > UINT16_MAX * 3 / 2) {
        return false;
    }
    set_count_no_check(ptr, count);
    return true;
}

static inline size_t get_count(HeapPointer ptr)
{
    return (ptr & COUNT_MASK) >> COUNT_OFFSET;
}

static inline void set_metadata(HeapPointer *ptr, size_t metadata)
{
    assert(metadata <= UINT8_MAX);
    *ptr = *ptr | (metadata << METADATA_OFFSET);
}

static inline size_t get_metadata(HeapPointer ptr)
{
    return (ptr & METADATA_MASK) >> METADATA_OFFSET;
}

static inline size_t get_location(HeapPointer ptr)
{
    return ptr & LOCATION_MASK;
}

static inline void gc_mark(HeapPointer *ptr)
{
    *ptr = *ptr | GC_MARK_MASK;
}

static inline void gc_unmark(HeapPointer *ptr)
{
    *ptr = *ptr & ~GC_MARK_MASK;
}

static inline bool gc_is_marked(HeapPointer ptr)
{
    return ((ptr & GC_MARK_MASK) >> GC_MARK_OFFSET);
}

static inline void set_array_bit(HeapPointer *ptr)
{
    *ptr = *ptr | ARRAY_BIT_MASK;
}

static inline bool is_array_ptr(HeapPointer ptr)
{
    return ptr & ARRAY_BIT_MASK;
}

static inline void set_array_obj_bit(HeapPointer *ptr)
{
    *ptr = *ptr | ARRAY_OBJ_BIT_MASK;
}

static inline bool is_array_of_objects(HeapPointer ptr)
{
    return ptr & ARRAY_OBJ_BIT_MASK;
}

#endif

