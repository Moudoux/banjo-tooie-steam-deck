// Inherited heap_realloc from the pinned Tooie decomp; repairs marked @recomp.
#include "core1/heap.h"
#include "memory.h"
#define TOOIE_RECOMP_PATCH __attribute__((section(".recomp_patch")))

_Static_assert(sizeof(void*) == 4 && sizeof(long) == 4 && sizeof(size_t) == 4,
               "heap patch requires MIPS o32 guest types");
_Static_assert(sizeof(struct HeapHeader) == 16 && sizeof(struct EmptyHeapBlock) == 32,
               "heap patch guest header layout changed");
void heap_resize_block(struct HeapHeader*, s32);
void func_8001B864(void*);
extern s32 D_8003F750;

TOOIE_RECOMP_PATCH void* heap_realloc(void* mem, s32 size) {
    struct HeapHeader* cur_block;
    void* new_mem;
    u32 old_capacity, copy_size;

    // Check if the current block can hold the new size
    cur_block = GET_BLOCK_HEADER(mem);
    // @recomp Retain the physical payload capacity before any resize.
    old_capacity = GET_BLOCK_MEM_SIZE(cur_block);
    if (GET_BLOCK_MEM_SIZE(cur_block) >= size) {
        // If so, resize it
        heap_resize_block(cur_block, size);
        return mem;
    }

    // Pin this block
    func_8001B864(mem);

    // Check if the next block is empty and whether merging it with this one will fit the requested size
    if (cur_block->next->state == 0 && (char*)cur_block->next->next - (char*)cur_block - sizeof(struct HeapHeader) >= size) {
        new_mem = cur_block->next;

        // Remove the next block from the free block list
        AS_EMPTY_BLOCK(new_mem)->next_free->prev_free = AS_EMPTY_BLOCK(new_mem)->prev_free;
        AS_EMPTY_BLOCK(new_mem)->prev_free->next_free = AS_EMPTY_BLOCK(new_mem)->next_free;

        // Update the allocated byte count
        D_8003F750 += GET_BLOCK_TOTAL_SIZE(&AS_EMPTY_BLOCK(new_mem)->header);

        // Remove the next block from the block list
        cur_block->next = AS_EMPTY_BLOCK(new_mem)->header.next;
        AS_EMPTY_BLOCK(new_mem)->header.next->prev = cur_block;

        // Resize the current block now that it's large enough to hold the requested size
        heap_resize_block(cur_block, size);

        // Unpin the original block
        func_8001B864(NULL);
        return mem;
    }

    // The current block couldn't be resized to fit the requested amount, allocate new memory to hold it
    new_mem = heap_alloc(size);
    if (new_mem == NULL) {
        // @recomp Failed growth must release the original pin.
        func_8001B864(NULL);
        return NULL;
    }

    // Copy the state from the original block into the new one
    GET_BLOCK_HEADER(new_mem)->unkC_28 = cur_block->unkC_28;
    GET_BLOCK_HEADER(new_mem)->unkC_29 = cur_block->unkC_29;

    // Copy the contents of the original block into the new one
    // @recomp Moving growth cannot read beyond the original payload.
    copy_size = func_8001BD30(size);
    aligned8_memcpy(new_mem, mem, copy_size < old_capacity ? copy_size : old_capacity);

    // Free and unpin the original block
    heap_free(mem);
    func_8001B864(NULL);
    return new_mem;
}
