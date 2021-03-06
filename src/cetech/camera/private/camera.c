#include <celib/cdb.h>
#include <cetech/ecs/ecs.h>
#include <cetech/transform/transform.h>
#include <celib/ydb.h>
#include <celib/ydb.h>
#include <cetech/camera/camera.h>

#include <celib/memory/allocator.h>
#include <celib/macros.h>
#include <celib/containers/array.h>
#include <celib/math/math.h>

#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/debugui/icons_font_awesome.h>
#include <cetech/editor/editor_ui.h>
#include <cetech/editor/property.h>

#include "celib/id.h"
#include "celib/config.h"
#include "celib/memory/memory.h"
#include "celib/api.h"

#include "celib/module.h"

#define _G CameraGlobal
static struct CameraGlobal {
    struct ce_alloc_t0 *allocator;
} CameraGlobal;

static void get_project_view(ct_world_t0 world,
                             struct ct_entity_t0 camera,
                             float *proj,
                             float *view,
                             int width,
                             int height) {

    struct ct_transform_comp *transform = ct_ecs_a0->get_one(world,
                                                             TRANSFORM_COMPONENT,
                                                             camera);

    struct ct_camera_component *camera_data = ct_ecs_a0->get_one(world,
                                                                 CAMERA_COMPONENT,
                                                                 camera);
    if(!transform) return;
    if(!camera_data) return;

    float fov = camera_data->fov;
    float near = camera_data->near;
    float far = camera_data->far;
    float *wworld = transform->world;

    float ratio = (float) (width) / (float) (height);

//    ce_mat4_look_at(view, transform->position,
//                    (float[]){0.0f, 0.0f, 1.0f},
//                    (float[]){0.0f, 1.0f, 0.0f});

    ce_mat4_proj_fovy(proj, fov,
                      ratio,
                      near,
                      far,
                      ct_gfx_a0->bgfx_get_caps()->homogeneousDepth);

    float w[16] = {};
    if (wworld) {
        ce_mat4_move(w, wworld);
    }

    w[12] *= -1.0f;
    w[13] *= -1.0f;
    w[14] *= -1.0f;

    ce_mat4_move(view, w);
}

static struct ct_camera_a0 camera_api = {
        .get_project_view = get_project_view,
};


struct ct_camera_a0 *ct_camera_a0 = &camera_api;

static uint64_t cdb_type() {
    return CAMERA_COMPONENT;
}

static const char *display_name() {
    return ICON_FA_CAMERA " Camera";
}

static void *get_interface(uint64_t name_hash) {
    if (EDITOR_COMPONENT == name_hash) {
        static struct ct_editor_component_i0 ct_editor_component_i0 = {
                .display_name = display_name,
        };

        return &ct_editor_component_i0;
    }

    return NULL;
}


static uint64_t size() {
    return sizeof(ct_camera_component);
}

static void camera_spawner(ct_world_t0 world,
                           uint64_t obj,
                           void* data) {

    const ce_cdb_obj_o0 * r = ce_cdb_a0->read(ce_cdb_a0->db(), obj);
    struct ct_camera_component *c = data;

    *c = (ct_camera_component) {
            .far = ce_cdb_a0->read_float(r, PROP_FAR, 100.0f),
            .near = ce_cdb_a0->read_float(r, PROP_NEAR, 0.0f),
            .fov = ce_cdb_a0->read_float(r, PROP_FOV, 60.0f),
    };
}


static struct ct_component_i0 ct_component_api = {
        .cdb_type = cdb_type,
        .size = size,
        .get_interface = get_interface,
        .spawner = camera_spawner,
};


static void property_editor(uint64_t obj, uint64_t context) {

    ct_editor_ui_a0->prop_float(obj,
                               PROP_NEAR,
                               "Near",
                               (ui_float_p0) {});

    ct_editor_ui_a0->prop_float(obj,
                                PROP_FAR,
                                "Far",
                                (ui_float_p0) {});

    ct_editor_ui_a0->prop_float(obj,
                                PROP_FOV,
                                "Fov",
                                (ui_float_p0) {});
}

static struct ct_property_editor_i0 property_editor_api = {
        .cdb_type = cdb_type,
        .draw_ui = property_editor,
};

static void _init(struct ce_api_a0 *api) {
    api->register_api(CT_CAMERA_API, &camera_api, sizeof(camera_api));

    _G = (struct _G) {
            .allocator = ce_memory_a0->system,
    };

    api->register_api(COMPONENT_INTERFACE, &ct_component_api, sizeof(ct_component_api));
    api->register_api(PROPERTY_EDITOR_INTERFACE, &property_editor_api, sizeof(property_editor_api));
}

static void _shutdown() {
}


CE_MODULE_DEF(
        camera,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ce_ydb_a0);
            CE_INIT_API(api, ce_ydb_a0);
            CE_INIT_API(api, ct_ecs_a0);
            CE_INIT_API(api, ce_cdb_a0);

        },
        {
            CE_UNUSED(reload);
            _init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);
            _shutdown();
        }
)