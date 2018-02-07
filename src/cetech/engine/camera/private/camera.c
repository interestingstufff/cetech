#include <cetech/engine/world/world.h>
#include <cetech/engine/renderer/renderer.h>
#include <cetech/engine/transform/transform.h>
#include <cetech/engine/camera/camera.h>
#include <cetech/core/yaml/yng.h>
#include <cetech/core/yaml/ydb.h>
#include <cetech/macros.h>
#include <cetech/core/containers/array.h>
#include <cetech/core/math/fmath.h>

#include "cetech/core/hashlib/hashlib.h"
#include "cetech/core/config/config.h"
#include "cetech/core/memory/memory.h"
#include "cetech/core/api/api_system.h"

#include "cetech/core/module/module.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_hashlib_a0);
CETECH_DECL_API(ct_yng_a0);
CETECH_DECL_API(ct_ydb_a0);
CETECH_DECL_API(ct_world_a0);
CETECH_DECL_API(ct_cdb_a0);


uint64_t combine(uint32_t a,
                 uint32_t b) {
    union {
        struct {
            uint32_t a;
            uint32_t b;
        };
        uint64_t ab;
    } c = {
            .a = a,
            .b = b,
    };

    return c.ab;
}


#define _G CameraGlobal
static struct CameraGlobal {
    uint64_t type;
    struct ct_alloc *allocator;
} CameraGlobal;

static int _camera_component_compiler(const char *filename,
                                      uint64_t *comp_key,
                                      uint32_t key_count,
                                      struct ct_cdb_writer_t *writer) {
    uint64_t keys[key_count + 1];
    memcpy(keys, comp_key, sizeof(uint64_t) * key_count);

    keys[key_count] = ct_yng_a0.key("near");
    float near = ct_ydb_a0.get_float(filename, CETECH_ARR_ARG(keys), 0.0f);

    keys[key_count] = ct_yng_a0.key("far");
    float far = ct_ydb_a0.get_float(filename, CETECH_ARR_ARG(keys), 0.0f);

    keys[key_count] = ct_yng_a0.key("fov");
    float fov = ct_ydb_a0.get_float(filename, CETECH_ARR_ARG(keys), 0.0f);

    ct_cdb_a0.set_float(writer, PROP_NEAR, near);
    ct_cdb_a0.set_float(writer, PROP_FAR, far);
    ct_cdb_a0.set_float(writer, PROP_FOV, fov);

    return 1;
}

static void get_project_view(struct ct_world world,
                             struct ct_entity camera,
                             float *proj,
                             float *view,
                             int width,
                             int height) {
    struct ct_cdb_obj_t *ent_obj = ct_world_a0.ent_obj(camera);

    float fov = ct_cdb_a0.read_float(ent_obj, PROP_FOV, 0.0f);
    float near = ct_cdb_a0.read_float(ent_obj, PROP_NEAR, 0.0f);
    float far = ct_cdb_a0.read_float(ent_obj, PROP_FAR, 0.0f);

    float ratio = (float) (width) / (float) (height);

    ct_mat4_proj_fovy(proj, fov, ratio, near, far, true);

    float w[16];
    ct_transform_a0.get_world_matrix(camera, w);

    //ct_mat4_move(view, w);
    ct_mat4_inverse(view, w);
}

static struct ct_camera_a0 camera_api = {
        .get_project_view = get_project_view,
};

static void _init(struct ct_api_a0 *api) {
    api->register_api("ct_camera_a0", &camera_api);

    _G = (struct _G) {
            .allocator = ct_memory_a0.main_allocator(),
            .type = CT_ID64_0("camera"),
    };

    ct_world_a0.register_component_compiler(_G.type,
                                      _camera_component_compiler);

    ct_world_a0.register_component(_G.type);
}

static void _shutdown() {
}


CETECH_MODULE_DEF(
        camera,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_hashlib_a0);
            CETECH_GET_API(api, ct_yng_a0);
            CETECH_GET_API(api, ct_ydb_a0);
            CETECH_GET_API(api, ct_world_a0);
            CETECH_GET_API(api, ct_cdb_a0);
        },
        {
            CT_UNUSED(reload);
            _init(api);
        },
        {
            CT_UNUSED(reload);
            CT_UNUSED(api);
            _shutdown();
        }
)