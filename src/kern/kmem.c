#include <agave/kmem.h>

extern uint8_t _end;
#define HEAP_START ((uintptr_t)&_end)

#ifndef HEAP_SIZE
#define HEAP_SIZE 0x4000000 // 64 MB
#endif

static block_header_t *heap_start = (block_header_t*)HEAP_START;

void kheap_init(void) {
    heap_start->size = HEAP_SIZE - sizeof(block_header_t);
    heap_start->free = true;
    heap_start->next = NULL;
}

void kmemcpy(void* dest, const void* src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
}

static void coalesce_next(block_header_t *block) {
    if (block->next && block->next->free) {
        block->size += sizeof(block_header_t) + block->next->size;
        block->next = block->next->next;
    }
} 

void* kmalloc(size_t size) {
    size = (size + 7) & ~7;
    block_header_t *curr = heap_start;

    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size >= size + sizeof(block_header_t) + 8) {
                block_header_t *new_block = (block_header_t*)((char*)curr + sizeof(block_header_t) + size);
                new_block->size = curr->size - size - sizeof(block_header_t);
                new_block->free = true;
                new_block->next = curr->next;
                curr->next = new_block;
                curr->size = size;
            }
            curr->free = false;
            return (char*)curr + sizeof(block_header_t);
        }
        curr = curr->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    block_header_t *block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    block->free = true;
    coalesce_next(block);

    block_header_t *curr = heap_start;
    while (curr && curr->next != block) curr = curr->next;
    if (curr && curr->free) coalesce_next(curr);
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    block_header_t *block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    if (block->size >= new_size) return ptr;

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;
    kmemcpy(new_ptr, ptr, block->size);
    kfree(ptr);
    return new_ptr;
}

void* kcalloc(size_t num, size_t size) {
    size_t total_size = num * size;
    if (num && total_size / num != size) return NULL;

    void *ptr = kmalloc(total_size);
    if (ptr) {
        unsigned char *p = ptr;
        for (size_t i = 0; i < total_size; i++) p[i] = 0;
    }
    return ptr;
}