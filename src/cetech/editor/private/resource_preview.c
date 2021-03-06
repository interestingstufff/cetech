#include <celib/memory/allocator.h>
#include <celib/cdb.h>
#include <cetech/ecs/ecs.h>
#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/debugui/debugui.h>
#include <cetech/camera/camera.h>
#include <cetech/transform/transform.h>
#include <cetech/controlers/keyboard.h>

#include <cetech/editor/resource_preview.h>
#include <cetech/editor/resource_browser.h>
#include <cetech/editor/editor.h>
#include <celib/containers/hash.h>
#include <celib/math/math.h>

#include <cetech/renderer/gfx.h>
#include <cetech/render_graph/render_graph.h>
#include <cetech/default_rg/default_rg.h>
#include <cetech/editor/dock.h>
#include <cetech/controlers/controlers.h>
#include <cetech/editor/selcted_object.h>
#include <cetech/editor/editor_ui.h>

#include "celib/id.h"
#include "celib/config.h"
#include "celib/memory/memory.h"
#include "celib/api.h"
#include "celib/module.h"


#define _G AssetPreviewGlobals

#define PREVIEW_PTR \
    CE_ID64_0("preview_ptr", 0x1e2c71526a2e8a11ULL)

typedef struct preview_instance {
    ct_world_t0 world;
    struct ct_entity_t0 camera_ent;
    struct ct_viewport_t0 viewport;
    struct ct_entity_t0 ent;
    uint64_t selected_object;
    uint64_t type;
    bool locked;
    bool free;
}preview_instance;

static struct _G {
    struct ce_alloc_t0 *allocator;

    struct preview_instance *instances;
    struct preview_instance *baground;

    bool visible;
    bool active;
} _G;

static struct preview_instance *_new_preview() {
    const uint32_t n = ce_array_size(_G.instances);
    for (uint32_t i = 0; i < n; ++i) {
        struct preview_instance *pi = &_G.instances[i];

        if (!pi->free) {
            continue;
        }

        return pi;
    }

    uint32_t idx = n;
    ce_array_push(_G.instances,
                  ((preview_instance) {}),
                  ce_memory_a0->system);

    struct preview_instance *pi = &_G.instances[idx];

    return pi;
}


static void fps_camera_update(ct_world_t0 world,
                              struct ct_entity_t0 camera_ent,
                              float dt,
                              float dx,
                              float dy,
                              float updown,
                              float leftright,
                              float speed,
                              bool fly_mode) {

//    CE_UNUSED(dx);
//    CE_UNUSED(dy);
//
//    float wm[16];
//
//    struct ct_transform_comp *transform;
//    transform = ct_ecs_a0->get_one(world, TRANSFORM_COMPONENT,
//                                   camera_ent);
//
//    ce_mat4_move(wm, transform->world);
//
//    float x_dir[4];
//    float z_dir[4];
//    ce_vec4_move(x_dir, &wm[0 * 4]);
//    ce_vec4_move(z_dir, &wm[2 * 4]);
//
//    if (!fly_mode) {
//        z_dir[1] = 0.0f;
//    }
//
//    // POS
//    float x_dir_new[3];
//    float z_dir_new[3];
//
//    ce_vec3_mul_s(x_dir_new, x_dir, dt * leftright * speed);
//    ce_vec3_mul_s(z_dir_new, z_dir, dt * updown * speed);
//
//
//    float pos[3] = {};
//    ce_vec3_add(transform->position, pos, x_dir_new);
//    ce_vec3_add(pos, pos, z_dir_new);

//
//    uint64_t ent_obj = camera_ent.h;
//    uint64_t components = ce_cdb_a0->read_subobject(ent_obj,
//                                                    ENTITY_COMPONENTS, 0);
//
//    uint64_t component = ce_cdb_a0->read_subobject(components,
//                                                   TRANSFORM_COMPONENT, 0);
//
//    ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(component);
//    ce_cdb_a0->set_vec3(w, PROP_POSITION, pos);
//    ce_cdb_a0->write_commit(w);
//
    // ROT
//    float rotation_around_world_up[4];
//    float rotation_around_camera_right[4];
//
//    local rotation_around_world_up = Quatf.from_axis_angle(Vec3f.unit_y(), -dx * dt * 100)
//    local rotation_around_camera_right = Quatf.from_axis_angle(x_dir, dy * dt * 100)
//    local rotation = rotation_around_world_up * rotation_around_camera_right
//
//    Transform.set_position(self.transform, pos)
//    Transform.set_rotation(self.transform, rot * rotation)
//    end
}

static struct ct_resource_preview_i0 *_get_asset_preview(uint64_t asset_type) {
    struct ct_resource_i0 *resource_i;
    resource_i = ct_resource_a0->get_interface(asset_type);

    if (!resource_i) {
        return NULL;
    }

    if (!resource_i->get_interface) {
        return NULL;
    }

    return resource_i->get_interface(RESOURCE_PREVIEW_I);
}

static void set_asset(preview_instance *pi,
                      uint64_t obj) {
    if (!pi) {
        return;
    }

    if (pi->locked) {
        return;
    }

    if (pi->selected_object == obj) {
        return;
    }

    if (pi->selected_object && pi->ent.h) {

        uint64_t prev_type = ce_cdb_a0->obj_type(ce_cdb_a0->db(),
                                                 pi->selected_object);

        struct ct_resource_preview_i0 *i;
        i = _get_asset_preview(prev_type);

        if (i) {
            if (i->unload) {
                i->unload(pi->selected_object, pi->world, pi->ent);
            }
        }

        ct_ecs_a0->destroy(pi->world, &pi->ent, 1);

        pi->ent.h = 0;
        pi->type = 0;
    }

    if (obj) {
        uint64_t type = ce_cdb_a0->obj_type(ce_cdb_a0->db(), obj);

        pi->type = type;

        struct ct_resource_preview_i0 *i;
        i = _get_asset_preview(type);
        if (i) {
            if (i->load) {
                pi->ent = i->load(obj, pi->world);
            }
        }
    }

    pi->selected_object = obj;
}

static void draw_menu(uint64_t dock) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
    struct preview_instance *pi = ce_cdb_a0->read_ptr(reader,
                                                      PREVIEW_PTR, NULL);

    ct_dock_a0->context_btn(dock);
    ct_debugui_a0->SameLine(0, -1);
    uint64_t locked_object = ct_editor_ui_a0->lock_selected_obj(dock,
                                                                pi->selected_object);

    pi->locked = false;

    if (locked_object) {
        pi->selected_object = locked_object;
        pi->locked = true;
    }
}

static void _draw_preview(preview_instance *pi,
                          ce_vec2_t size) {

    if(!pi->type) {
        return;
    }

    struct ct_resource_preview_i0 *i;
    i = _get_asset_preview(pi->type);

    if(!i) {
        return;
    }

    if (i->draw_raw) {
        i->draw_raw(pi->selected_object, size);
    } else {

        struct ct_rg_builder_t0 *builder;
        builder = ct_renderer_a0->viewport_builder(pi->viewport);

        builder->set_size(builder, size.x, size.y);

        bgfx_texture_handle_t th;
        th = builder->get_texture(builder, RG_OUTPUT_TEXTURE);

        ct_debugui_a0->Image(th,
                             &size,
                             &(ce_vec4_t) {1.0f, 1.0f, 1.0f, 1.0f},
                             &(ce_vec4_t) {0.0f, 0.0f, 0.0, 0.0f});
    }
}

static void on_debugui(uint64_t dock) {
    _G.active = ct_debugui_a0->IsMouseHoveringWindow();

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), dock);
    struct preview_instance *pi = ce_cdb_a0->read_ptr(reader,
                                                      PREVIEW_PTR, NULL);

    if (!pi) {
        return;
    }

    const uint64_t context = ce_cdb_a0->read_uint64(reader, PROP_DOCK_CONTEXT,
                                                    0);
    set_asset(pi, ct_selected_object_a0->selected_object(context));

    ce_vec2_t size = ct_debugui_a0->GetContentRegionAvail();
    _draw_preview(pi, size);
}


static bool init() {
    _G.visible = true;

    struct preview_instance *pi = _new_preview();
    _G.baground = pi;
    pi->world = ct_ecs_a0->create_world();
    pi->camera_ent = ct_ecs_a0->spawn(pi->world, 0x57899875c4457313);
    pi->viewport = ct_renderer_a0->create_viewport(pi->world, pi->camera_ent);

    ct_dock_a0->create_dock(RESOURCE_PREVIEW_I, true);
    return true;
}

static void update(float dt) {
    uint32_t n = ce_array_size(_G.instances);
    for (int i = 0; i < n; ++i) {
        struct preview_instance *pi = &_G.instances[i];

        ct_ecs_a0->simulate(pi->world, dt);

        uint64_t selected_object = pi->selected_object;
        if (!selected_object) {
            return;
        }

        struct ct_controlers_i0 *keyboard;
        keyboard = ct_controlers_a0->get(CONTROLER_KEYBOARD);

        if (_G.active) {
            float updown = 0.0f;
            float leftright = 0.0f;

            uint32_t up_key = keyboard->button_index("w");
            uint32_t down_key = keyboard->button_index("s");
            uint32_t left_key = keyboard->button_index("a");
            uint32_t right_key = keyboard->button_index("d");

            if (keyboard->button_state(0, up_key) > 0) {
                updown = 1.0f;
            }

            if (keyboard->button_state(0, down_key) > 0) {
                updown = -1.0f;
            }

            if (keyboard->button_state(0, right_key) > 0) {
                leftright = 1.0f;
            }

            if (keyboard->button_state(0, left_key) > 0) {
                leftright = -1.0f;
            }

            fps_camera_update(pi->world, pi->camera_ent, dt,
                              0, 0, updown, leftright, 10.0f, false);
        }
    }
}

void set_background_resource(ct_resource_id_t0 resource) {
    set_asset(_G.baground, resource.uid);
}

void draw_background_texture(ce_vec2_t size) {
    _draw_preview(_G.baground, size);
}

static struct ct_resource_preview_a0 asset_preview_api = {
        .set_background_resource = set_background_resource,
        .draw_background_texture = draw_background_texture,
};

struct ct_resource_preview_a0 *ct_resource_preview_a0 = &asset_preview_api;

static const char *dock_title() {
    return "Resource preview";
}

static const char *name(uint64_t dock) {
    return "asset_preview";
}

static uint64_t cdb_type() {
    return RESOURCE_PREVIEW_I;
};

static void open(uint64_t dock) {
    struct preview_instance *pi = _new_preview();

    pi->world = ct_ecs_a0->create_world();
    pi->camera_ent = ct_ecs_a0->spawn(pi->world, 0x57899875c4457313);
    pi->viewport = ct_renderer_a0->create_viewport(pi->world, pi->camera_ent);

    ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), dock);
    ce_cdb_a0->set_ptr(w, PREVIEW_PTR, pi);
    ce_cdb_a0->write_commit(w);
}

static struct ct_dock_i0 dock_api = {
        .cdb_type = cdb_type,
        .display_title = dock_title,
        .name = name,
        .draw_ui = on_debugui,
        .draw_menu = draw_menu,
        .open = open,
};


static struct ct_editor_module_i0 ct_editor_module_api= {
        .init = init,
        .update = update,
};


static void _init(struct ce_api_a0 *api) {


    _G = (struct _G) {
            .allocator = ce_memory_a0->system
    };

    api->register_api(DOCK_INTERFACE, &dock_api, sizeof(dock_api));
    api->register_api(CT_ASSET_PREVIEW_API, &asset_preview_api, sizeof(asset_preview_api));
    api->register_api(EDITOR_MODULE_INTERFACE, &ct_editor_module_api, sizeof(ct_editor_module_api));
}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        asset_preview,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ct_debugui_a0);
            CE_INIT_API(api, ct_ecs_a0);
            CE_INIT_API(api, ct_camera_a0);
            CE_INIT_API(api, ce_cdb_a0);
            CE_INIT_API(api, ct_rg_a0);
            CE_INIT_API(api, ct_default_rg_a0);
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