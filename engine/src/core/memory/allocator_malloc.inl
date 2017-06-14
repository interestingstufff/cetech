#ifndef CETECH_ALLOCATOR_MALLOC_H
#define CETECH_ALLOCATOR_MALLOC_H

#include <stdint.h>

#include <cetech/core/log.h>
#include <cetech/celib/allocator.h>
#include <cetech/core/errors.h>

#include "header.h"


#define MAX_MEM_TRACE 1024

static uint32_t size_with_padding(uint32_t size,
                                  uint32_t align) {
    return size + align + sizeof(struct Header);
}

struct allocator_malloc {
    struct allocator base;
    uint32_t total_allocated;
    struct allocator_trace_entry trace[MAX_MEM_TRACE];
};

void *malloc_allocator_allocate(struct allocator *allocator,
                                uint32_t size,
                                uint32_t align) {
    struct allocator_malloc *a = (struct allocator_malloc *) allocator;

    const uint32_t ts = size_with_padding(size, align);
    struct Header *h = (struct Header *) memory::malloc(ts);

    void *p = data_pointer(h, align);
    fill(h, p, ts);
    a->total_allocated += ts;

    allocator_trace_pointer(a->trace, MAX_MEM_TRACE, p);

    return p;
}

void malloc_allocator_deallocate(struct allocator *allocator,
                                 void *p) {
    struct allocator_malloc *a = (struct allocator_malloc *) allocator;

    if (!p)
        return;

    struct Header *h = header(p);
    a->total_allocated -= h->size;

    memory::allocator_stop_trace_pointer(a->trace, MAX_MEM_TRACE, p);

    memory::free(h);
}

uint32_t malloc_allocator_allocated_size(void *p) {
    return header(p)->size;
}

uint32_t malloc_allocator_total_allocated(struct allocator *allocator) {
    struct allocator_malloc *a = (struct allocator_malloc *) allocator;

    return a->total_allocated;
}

struct allocator *malloc_allocator_create() {
    struct allocator_malloc *m = (allocator_malloc *) memory::malloc(sizeof(struct allocator_malloc));

    m->base = (struct allocator) {
            .allocate = malloc_allocator_allocate,
            .deallocate = malloc_allocator_deallocate,
            .total_allocated = malloc_allocator_total_allocated,
            .allocated_size = malloc_allocator_allocated_size
    };

    m->total_allocated = 0;

    return (struct allocator *) m;
}

void malloc_allocator_destroy(struct allocator *a) {
    struct allocator_malloc *m = (struct allocator_malloc *) a;

    memory::allocator_check_trace(m->trace, MAX_MEM_TRACE);

    //CETECH_ASSERT("memory.malloc", m->total_allocated == 0);
    memory::free(m);
}

#endif //CETECH_ALLOCATOR_MALLOC_H
