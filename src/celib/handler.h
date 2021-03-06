#ifndef CE_HANDLER_H
#define CE_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "celib_types.h"

#include "celib/containers/array.h"


typedef struct ce_handler_t0 {
    char *_generation;
    uint64_t *_free_idx;
} ce_handler_t0;

#define _GENBITCOUNT   8
#define _INDEXBITCOUNT 54
#define _MINFREEINDEXS 1024

#define handler_idx(h) ((h) >> _GENBITCOUNT)
#define handler_gen(h) ((h) & ((1 << (_GENBITCOUNT - 1))))

static inline uint64_t ce_handler_create(ce_handler_t0 *handler,
                                         const ce_alloc_t0 *allocator) {
    uint64_t idx;

    if (ce_array_size(handler->_free_idx) > _MINFREEINDEXS) {
        idx = handler->_free_idx[0];
        ce_array_pop_front(handler->_free_idx);
    } else {
        ce_array_push(handler->_generation, 0, allocator);

        idx = ce_array_size(handler->_generation) - 1;
    }

    uint64_t hid = ((idx) << _GENBITCOUNT) | (handler->_generation[idx]);

    return hid;
}

static inline void ce_handler_destroy(ce_handler_t0 *handler,
                                      uint64_t handlerid,
                                      const ce_alloc_t0 *allocator) {
    uint64_t id = handler_idx(handlerid);

    handler->_generation[id] += 1;
    ce_array_push(handler->_free_idx, id, allocator);
}

static inline bool ce_handler_alive(ce_handler_t0 *handler,
                                    uint64_t handlerid) {
    return handler->_generation[handler_idx(handlerid)] ==
           handler_gen(handlerid);
}

static inline void ce_handler_free(ce_handler_t0 *handler,
                                   const ce_alloc_t0 *allocator) {
    ce_array_free(handler->_free_idx, allocator);
    ce_array_free(handler->_generation, allocator);
}

#ifdef __cplusplus
};
#endif

#endif //CE_HANDLER_H
