#ifndef CETECH_RESOURCE_INTERNAL_H
#define CETECH_RESOURCE_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cetech/kernel/hash.h>
#include <cetech/modules/resource.h>

#ifdef CETECH_CAN_COMPILE

void resource_compiler_register(uint64_t type,
                                resource_compilator_t compilator);

void resource_compiler_compile_all();

int resource_compiler_get_filename(char *filename,
                                   size_t max_ken,
                                   uint64_t type,
                                   uint64_t name);

const char *resource_compiler_get_source_dir();


char *resource_compiler_get_tmp_dir(allocator *alocator,
                                    const char *platform);

char *resource_compiler_external_join(allocator *alocator,
                                      const char *name);

void resource_compiler_create_build_dir(struct config_api_v0 config,
                                        struct app_api_v0 app);

const char *resource_compiler_get_core_dir();

#endif


char *resource_compiler_get_build_dir(allocator *a,
                                      const char *platform);

#ifdef __cplusplus
}
#endif

int package_init(struct api_v0 *api);

void package_shutdown();

void package_load(uint64_t name);

void package_load(uint64_t name);

void package_unload(uint64_t name);

int package_is_loaded(uint64_t name);

void package_flush(uint64_t name);

#endif //CETECH_RESOURCE_INTERNAL_H
