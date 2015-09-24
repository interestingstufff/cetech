#pragma once

#include "common/memory/memory.h"
#include "common/container/container_types.h"
#include "rapidjson/document.h"

namespace cetech {
    class DevelopManager {
        public:
            virtual void clear() = 0;
            virtual void send_buffer() = 0;

            virtual EventStream& event_stream() = 0;

            virtual void push_begin_frame() = 0;
            virtual void push_end_frame() = 0;
            virtual void push_record_float(const char* name, const float value) = 0;

            static DevelopManager* make(Allocator& alocator);
            static void destroy(Allocator& alocator, DevelopManager* dm);
    };
}