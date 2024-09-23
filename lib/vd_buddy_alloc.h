#ifndef VD_BUDDY_ALLOC_H
#define VD_BUDDY_ALLOC_H
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_BUDDY_ALLOC_IMPLEMENTATION
#endif

#define VD_BUDDY_ALLOCATION_PROC(name) void *name(void *ptr, size_t prevsize, size_t newsize, void *usrdata)
typedef VD_BUDDY_ALLOCATION_PROC(VD_BuddyAllocationProc);

typedef struct {
	size_t	size;
	int		is_free;
    int     reserved;
} VD_BuddyBlock;

typedef struct {
	VD_BuddyAllocationProc	*proc;
	void					*usrdata;
	VD_BuddyBlock			*head;
	VD_BuddyBlock			*tail;
    size_t                   alignment;
} VD_BuddyAlloc;

VD_BUDDY_ALLOCATION_PROC(vd_buddy_alloc_default_palloc);
void vd_buddy_alloc_init(VD_BuddyAlloc *alloc, VD_BuddyAllocationProc *palloc, void *usrdata, size_t initial_size, size_t alignment);
void *vd_buddy_alloc_realloc(VD_BuddyAlloc *alloc, void *ptr, size_t size);
void vd_buddy_alloc_deinit(VD_BuddyAlloc *alloc);
void vd_buddy_alloc_get_stats(
    VD_BuddyAlloc *alloc,
    size_t *used,
    size_t *total,
    size_t *num_blocks,
    size_t *num_free_blocks);

#endif // !VD_BUDDY_ALLOC_H

#ifdef VD_BUDDY_ALLOC_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>

VD_BUDDY_ALLOCATION_PROC(vd_buddy_alloc_default_palloc) {
    if (newsize == 0) {
        free((void *)ptr);
        return 0;
    }
    else {
        return (void *)realloc((void *)ptr, newsize);
    }
}


static size_t vd_buddy_alloc__round_up_to_next_power_of_2(size_t x)
{
    if (x == 0) {
        return 1;
    }

    if ((x & (x - 1)) == 0) {
        return x;
    }

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    if (sizeof(size_t) > 4) {
        x |= x >> 32;
    }

    return x + 1;
}


static int vd_buddy_alloc__is_power_of_two(size_t n)
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

static size_t vd_buddy_alloc__align_forward(size_t ptr, size_t align)
{
    size_t p, a, modulo;

    assert(vd_buddy_alloc__is_power_of_two(align));

    p = ptr;
    a = (size_t)align;
    // Same as (p % a) but faster as 'a' is a power of two
    modulo = p & (a - 1);

    if (modulo != 0) {
        // If 'p' address is not aligned, push the address to the
        // next value which is aligned
        p += a - modulo;
    }
    return p;
}

static VD_BuddyBlock *vd_buddy_alloc__next_block(VD_BuddyBlock *block)
{
    return (VD_BuddyBlock *)((char *)block + block->size);
}

static VD_BuddyBlock *vd_buddy_alloc__split(VD_BuddyBlock *block, size_t size)
{
    if (block == 0 || size == 0) {
        return 0;
    }

    while (size < block->size) {
        size_t sz = block->size >> 1;
        block->size = sz;
        block = vd_buddy_alloc__next_block(block);
        block->size = sz;
        block->is_free = 1;
    }

    if (size <= block->size) {
        return block;
    }

    return 0;
}

static VD_BuddyBlock *vd_buddy_alloc__find(VD_BuddyBlock *head, VD_BuddyBlock *tail, size_t size)
{
    VD_BuddyBlock *best = 0;
    VD_BuddyBlock *block = head;
    VD_BuddyBlock *buddy = vd_buddy_alloc__next_block(block);

    if (buddy == tail && block->is_free) {
        return vd_buddy_alloc__split(block, size);
    }

    while (block < tail && buddy < tail) {
        if (block->is_free && buddy->is_free && block->size == buddy->size) {
            block->size <<= 1;

            if (size <= block->size && (best == 0 || block->size <= best->size)) {
                best = block;
            }

            block = vd_buddy_alloc__next_block(buddy);
            if (block < tail) {
                buddy = vd_buddy_alloc__next_block(block);
            }

            continue;
        }

        if (block->is_free && size <= block->size &&
            (best == 0 || block->size <= best->size)) 
        {
            best = block;
        }

        if (buddy->is_free && size <= buddy->size &&
            (best == 0 || buddy->size < best->size))
        {
            best = buddy;
        }

        if (block->size <= buddy->size) {
            block = vd_buddy_alloc__next_block(buddy);

            if (block < tail) {
                buddy = vd_buddy_alloc__next_block(block);
            }
        } else {
            block = buddy;
            buddy = vd_buddy_alloc__next_block(buddy);
        }
    }

    if (best != 0) {
        return vd_buddy_alloc__split(best, size);
    }

    return 0;
}

static size_t vd_buddy_alloc__size_req(VD_BuddyAlloc *alloc, size_t size) {
    size_t actual_size = alloc->alignment;
    size += sizeof(VD_BuddyBlock);
    size = vd_buddy_alloc__align_forward(size, alloc->alignment);

    while (size > actual_size) {
        actual_size <<= 1;
    }

    return actual_size;
}

static void vd_buddy_alloc__coalescence(VD_BuddyBlock *head, VD_BuddyBlock *tail) {
    for (;;) {
        VD_BuddyBlock *block = head;
        VD_BuddyBlock *buddy = vd_buddy_alloc__next_block(block);

        int no_coalescence = 1;

        while (block < tail && buddy < tail) {
            if (block->is_free && buddy->is_free && block->size == buddy->size) {
                block->size <<= 1;
                block = vd_buddy_alloc__next_block(block);
                if (block < tail) {
                    buddy = vd_buddy_alloc__next_block(block);
                    no_coalescence = 0;
                }
            } else if (block->size < buddy->size) {
                block = buddy;
                buddy = vd_buddy_alloc__next_block(buddy);
            } else {
                block = vd_buddy_alloc__next_block(buddy);
                if (block < tail) {
                    buddy = vd_buddy_alloc__next_block(block);
                }
            }
        }

        if (no_coalescence) {
            return;
        }
    }
}

void vd_buddy_alloc_init(VD_BuddyAlloc *alloc, VD_BuddyAllocationProc *palloc, void *usrdata, size_t initial_size, size_t alignment)
{
    assert(vd_buddy_alloc__is_power_of_two(sizeof(VD_BuddyBlock)));
    alignment           = vd_buddy_alloc__round_up_to_next_power_of_2(alignment);
    initial_size        = vd_buddy_alloc__round_up_to_next_power_of_2(initial_size);
	
	void *memory = palloc(0, 0, initial_size, usrdata);
    alloc->proc             = palloc;
    alloc->usrdata          = usrdata;
    alloc->head             = (VD_BuddyBlock *)memory;
    alloc->head->size       = initial_size;
    alloc->head->is_free    = 1;
    alloc->tail             = vd_buddy_alloc__next_block(alloc->head);
    alloc->alignment        = alignment < sizeof(VD_BuddyBlock) ? sizeof(VD_BuddyBlock) : alignment;
}

void *vd_buddy_alloc_realloc(VD_BuddyAlloc *alloc, void *ptr, size_t size)
{
    if (ptr == 0) {
        size_t actual_size = vd_buddy_alloc__size_req(alloc, size);

        VD_BuddyBlock *found = vd_buddy_alloc__find(alloc->head, alloc->tail, actual_size);
        if (found == 0) {
            vd_buddy_alloc__coalescence(alloc->head, alloc->tail);
            found = vd_buddy_alloc__find(alloc->head, alloc->tail, actual_size);
        }

        if (found != 0) {
            found->is_free = 0;
            return (void *)((char *)found + alloc->alignment);
        }

        return 0;
    } else if (ptr != 0 && size == 0) {
        assert(alloc->head <= ptr);
        assert(ptr <= alloc->tail);

        VD_BuddyBlock *block = (VD_BuddyBlock *)((char *)ptr - alloc->alignment);
        block->is_free = 1;

        return 0;
    } else {
        return 0;
    }
}

void vd_buddy_alloc_deinit(VD_BuddyAlloc *alloc)
{
    alloc->proc(alloc->head, alloc->head->size, 0, alloc->usrdata);
}

void vd_buddy_alloc_get_stats(
    VD_BuddyAlloc *alloc,
    size_t *used,
    size_t *total,
    size_t *num_blocks,
    size_t *num_free_blocks)
{
    size_t used_bytes = 0;
    size_t total_bytes = 0;
    size_t num_blocks_ = 0;
    size_t num_free_blocks_ = 0;

    VD_BuddyBlock *block = alloc->head;
    VD_BuddyBlock *tail = alloc->tail;

    while (block < tail) {
        total_bytes += block->size;
        num_blocks_++;

        if (block->is_free) {
            num_free_blocks_++;
        } else {
            used_bytes += block->size;
        }

        block = vd_buddy_alloc__next_block(block);
    }

    if (used != 0) {
        *used = used_bytes;
    }

    if (total != 0) {
        *total = total_bytes;
    }

    if (num_blocks != 0) {
        *num_blocks = num_blocks_;
    }

    if (num_free_blocks != 0) {
        *num_free_blocks = num_free_blocks_;
    }
}

#endif