#include <include/SDL2/SDL_cpuinfo.h>
#include <cetech/core/os/cpu.h>
#include <cetech/core/module/module.h>
#include <cetech/core/api/api_system.h>
#include "celib/macros.h"

int cpu_count() {
    return SDL_GetCPUCount();
}

static ct_cpu_a0 cpu_api = {
        .count = cpu_count
};

CETECH_MODULE_DEF(
        cpu,
        {
            CEL_UNUSED(api);
        },
        {
            api->register_api("ct_cpu_a0", &cpu_api);
        },
        {
            CEL_UNUSED(api);
        }
)
