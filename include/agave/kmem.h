#ifndef AGAVE_KMEM_H
#define AGAVE_KMEM_H

#include <stddef.h>
#include <stdbool.h>

typedef struct block_header {
    size_t size;
    bool free;
    struct block_header *next;
} block_header_t;

void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t new_size);
void *kcalloc(size_t num, size_t size);
void kmemcpy(void* dest, const void* src, size_t n);

#endif // AGAVE_KMEM_H