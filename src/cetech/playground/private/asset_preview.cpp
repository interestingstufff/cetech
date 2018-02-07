#include <cetech/engine/world/world.h>
#include <cetech/engine/renderer/renderer.h>
#include <cetech/engine/renderer/texture.h>
#include <cetech/engine/debugui/debugui.h>
#include <cetech/engine/camera/camera.h>
#include <cetech/engine/transform/transform.h>

#include <cetech/engine/input/input.h>
#include <cetech/engine/renderer/viewport.h>
#include <cetech/playground/asset_preview.h>
#include <cetech/playground/asset_browser.h>
#include <cetech/playground/playground.h>
#include <cetech/core/containers/hash.h>
#include <cetech/core/math/fmath.h>
#include "cetech/core/containers/map.inl"

#include "cetech/core/hashlib/hashlib.h"
#include "cetech/core/config/config.h"
#include "cetech/core/memory/memory.h"
#include "cetech/core/api/api_system.h"
#include "cetech/core/module/module.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_hashlib_a0);
CETECH_DECL_API(ct_debugui_a0);
CETECH_DECL_API(ct_world_a0);
CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_keyboard_a0);
CETECH_DECL_API(ct_camera_a0);
CETECH_DECL_API(ct_viewport_a0);
CETECH_DECL_API(ct_asset_browser_a0);
CETECH_DECL_API(ct_playground_a0);
CETECH_DECL_API(ct_cdb_a0);

using namespace celib;

#define _G AssetPreviewGlobals

static struct _G {
    ct_alloc *allocator;
    struct ct_hash_t preview_fce_map;
    ct_asset_preview_fce *preview_fce;

    uint64_t active_type;
    uint64_t active_name;
    const char *active_path;

    ct_viewport viewport;
    ct_world world;
    ct_entity camera_ent;
    bool visible;
    bool active;
} _G;


static void fps_camera_update(ct_world world,
                              ct_entity camera_ent,
                              float dt,
                              float dx,
                              float dy,
                              float updown,
                              float leftright,
                              float speed,
                              bool fly_mode) {

    CT_UNUSED(dx);
    CT_UNUSED(dy);

    float pos[3];
    float rot[3];
    float wm[16];

    ct_cdb_obj_t *obj = ct_world_a0.ent_obj(camera_ent);

    ct_cdb_a0.read_vec3(obj, PROP_POSITION, pos);
    ct_cdb_a0.read_vec3(obj, PROP_ROTATION, rot);

    ct_transform_a0.get_world_matrix(camera_ent, wm);

    float x_dir[4];
    float z_dir[4];
    ct_vec4_move(x_dir, &wm[0 * 4]);
    ct_vec4_move(z_dir, &wm[2 * 4]);

    if (!fly_mode) {
        z_dir[1] = 0.0f;
    }

    // POS
    float x_dir_new[3];
    float z_dir_new[3];

    ct_vec3_mul_s(x_dir_new, x_dir, dt * leftright * speed);
    ct_vec3_mul_s(z_dir_new, z_dir, dt * updown * speed);

    ct_vec3_add(pos, pos, x_dir_new);
    ct_vec3_add(pos, pos, z_dir_new);

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

    ct_cdb_writer_t *w = ct_cdb_a0.write_begin(obj);
    ct_cdb_a0.set_vec3(w, PROP_POSITION, pos);
    ct_cdb_a0.write_commit(w);
}

static void on_debugui() {
    if (ct_debugui_a0.BeginDock("Asset preview", &_G.visible,
                                DebugUIWindowFlags_NoScrollbar)) {

        _G.active = ct_debugui_a0.IsMouseHoveringWindow();

        auto th = ct_viewport_a0.get_local_resource(_G.viewport,
                                                    CT_ID64_0("bb_color"));

        float size[2];
        ct_debugui_a0.GetWindowSize(size);
        ct_viewport_a0.resize(_G.viewport, size[0], size[1]);
        ct_debugui_a0.Image2(th,
                             size,
                             (float[2]) {0.0f, 0.0f},
                             (float[2]) {1.0f, 1.0f},
                             (float[4]) {1.0f, 1.0f, 1.0f, 1.0f},
                             (float[4]) {0.0f, 0.0f, 0.0, 0.0f});
    }
    ct_debugui_a0.EndDock();

}

static void render() {
    if (_G.visible) {
        ct_viewport_a0.render_world(_G.world, _G.camera_ent, _G.viewport);
    }
}

static void set_asset(uint64_t type,
                      uint64_t name,
                      uint64_t root,
                      const char *path) {
    CT_UNUSED(root);

    if (_G.active_name == name) {
        return;
    }

    uint64_t idx = ct_hash_lookup(&_G.preview_fce_map, _G.active_type,
                                  UINT64_MAX);
    if (idx != UINT64_MAX) {
        ct_asset_preview_fce fce = _G.preview_fce[idx];

        if (fce.unload) {
            fce.unload(_G.active_path, _G.active_type, _G.active_name,
                       _G.world);
        }
    }

    _G.active_type = type;
    _G.active_name = name;
    _G.active_path = path;

    idx = ct_hash_lookup(&_G.preview_fce_map, type, UINT64_MAX);
    if (idx != UINT64_MAX) {
        ct_asset_preview_fce fce = _G.preview_fce[idx];

        if (fce.load) {
            fce.load(path, type, name, _G.world);
        }
    }

    ct_cdb_obj_t *obj = ct_world_a0.ent_obj(_G.camera_ent);
    ct_cdb_writer_t *w = ct_cdb_a0.write_begin(obj);
    ct_cdb_a0.set_vec3(w, PROP_POSITION, (float[3]) {0.0f, 0.0f, -10.0f});
    ct_cdb_a0.write_commit(w);
}

static void init() {
    _G.visible = true;
    _G.viewport = ct_viewport_a0.create(CT_ID64_0("default"), 0, 0);
    _G.world = ct_world_a0.create_world();
    _G.camera_ent = ct_world_a0.spawn_entity(_G.world, CT_ID64_0("content/camera"));
}

static void shutdown() {

}

static void update(float dt) {
    if (!_G.active) {
        return;
    }

    float updown = 0.0f;
    float leftright = 0.0f;

    auto up_key = ct_keyboard_a0.button_index("w");
    auto down_key = ct_keyboard_a0.button_index("s");
    auto left_key = ct_keyboard_a0.button_index("a");
    auto right_key = ct_keyboard_a0.button_index("d");

    if (ct_keyboard_a0.button_state(0, up_key) > 0) {
        updown = 1.0f;
    }

    if (ct_keyboard_a0.button_state(0, down_key) > 0) {
        updown = -1.0f;
    }

    if (ct_keyboard_a0.button_state(0, right_key) > 0) {
        leftright = 1.0f;
    }

    if (ct_keyboard_a0.button_state(0, left_key) > 0) {
        leftright = -1.0f;
    }

    fps_camera_update(_G.world, _G.camera_ent, dt,
                      0, 0, updown, leftright, 10.0f, false);
}

#define ct_instance_map(a, h, k, item, al) \
    ct_array_push(a, item, al); \
    ct_hash_add(h, k, ct_array_size(a) - 1, al)

void register_type_preview(uint64_t type,
                           ct_asset_preview_fce fce) {
    ct_instance_map(_G.preview_fce, &_G.preview_fce_map, type, fce,
                    _G.allocator);
}

void unregister_type_preview(uint64_t type) {
    uint64_t idx = ct_hash_lookup(&_G.preview_fce_map, type, UINT64_MAX);
    if (UINT64_MAX == idx) {
        return;
    }
    ct_hash_remove(&_G.preview_fce_map, type);
}

static void on_menu_window() {
    ct_debugui_a0.MenuItem2("Asset preview", NULL, &_G.visible, true);
}

static ct_asset_preview_a0 asset_preview_api = {
        .register_type_preview = register_type_preview,
        .unregister_type_preview = unregister_type_preview
};

static void _init(ct_api_a0 *api) {
    _G = (struct _G) {
            .allocator = ct_memory_a0.main_allocator()
    };

    ct_playground_a0.register_module(
            CT_ID64_0("asset_preview"),
            (ct_playground_module_fce) {
                    .on_init = init,
                    .on_shutdown = shutdown,
                    .on_render = render,
                    .on_update = update,
                    .on_ui = on_debugui,
                    .on_menu_window = on_menu_window,
            });

    ct_asset_browser_a0.register_on_asset_click(set_asset);

    api->register_api("ct_asset_preview_a0", &asset_preview_api);
}

static void _shutdown() {
    ct_playground_a0.unregister_module(CT_ID64_0("asset_preview"));
    ct_asset_browser_a0.unregister_on_asset_click(set_asset);

    ct_hash_free(&_G.preview_fce_map, _G.allocator);
    ct_array_free(_G.preview_fce, _G.allocator);

    _G = {};
}

CETECH_MODULE_DEF(
        asset_preview,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_hashlib_a0);
            CETECH_GET_API(api, ct_debugui_a0);
            CETECH_GET_API(api, ct_world_a0);
            CETECH_GET_API(api, ct_camera_a0);
            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_keyboard_a0);
            CETECH_GET_API(api, ct_viewport_a0);
            CETECH_GET_API(api, ct_asset_browser_a0);
            CETECH_GET_API(api, ct_playground_a0);
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