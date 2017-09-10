#include <celib/macros.h>

#include <cetech/kernel/log.h>
#include <cetech/kernel/config.h>
#include <cetech/kernel/module.h>
#include <cetech/kernel/api_system.h>
#include <cetech/kernel/hashlib.h>

#include <cetech/modules/input/input.h>
#include <cetech/modules/application/application.h>
#include <cetech/modules/entity/entity.h>

#include <cetech/modules/playground//playground.h>
#include <cetech/modules/debugui/debugui.h>
#include <cetech/modules/renderer/renderer.h>
#include <cetech/modules/transform/transform.h>
#include <cetech/modules/camera/camera.h>
#include <cetech/modules/level/level.h>
#include <cetech/modules/renderer/viewport.h>
#include <cetech/modules/renderer/texture.h>

CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_app_a0);
CETECH_DECL_API(ct_keyboard_a0);
CETECH_DECL_API(ct_playground_a0);
CETECH_DECL_API(ct_debugui_a0);
CETECH_DECL_API(ct_hash_a0);

CETECH_DECL_API(ct_renderer_a0);

CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_world_a0);
CETECH_DECL_API(ct_entity_a0);
CETECH_DECL_API(ct_camera_a0);
CETECH_DECL_API(ct_level_a0);
CETECH_DECL_API(ct_texture_a0);


static struct G {
    ct_viewport viewport;
    ct_world world;
    ct_camera camera;
    ct_entity camera_ent;
    float dt;
} _G;


void update(float dt) {
    _G.dt = dt;

    if (ct_keyboard_a0.button_state(0, ct_keyboard_a0.button_index("v"))) {
        ct_log_a0.info("example", "PO");
        ct_log_a0.error("example", "LICE");
    }
    ///ct_log_a0.debug("example", "%f", dt);
}

void module1() {
    static bool visible = true;
    if (ct_debugui_a0.BeginDock("Module 1", &visible,
                                DebugUIWindowFlags_Empty)) {

        ct_debugui_a0.Text("DT: %f", _G.dt);
        ct_debugui_a0.Text("FPS: %f", 1.0f / _G.dt );

        ct_debugui_a0.Text("Random FPS: %f", static_cast<double>(rand()));

        ct_debugui_a0.Text("xknalsnxlsanlknxlasnlknxslknsaxdear imgui, %d", 111);

            static float v[2] = {100.0f, 100.0f};
            if(ct_debugui_a0.Button("wwwwww", v)) {
                ct_log_a0.info("dasdsa", "dsadkjjkhjkbjhkjbjkbjkksadsadsad");
            }

        static float col[4] = {0.0f, 1.0f, 0.0f, 0.0f};
        ct_debugui_a0.ColorButton(col, 1, 2);

        static float vv;
        ct_debugui_a0.DragFloat("FOO:", &vv, 1.0f, 0.0f, 10000.0f, "%.3f",
                                1.0f);

        static float col2[4] = {0.0f, 1.0f, 0.0f, 0.0f};
        ct_debugui_a0.ColorEdit3("COLOR", col2);

        static float col3[4] = {0.0f, 1.0f, 0.0f, 0.0f};
        ct_debugui_a0.ColorWheel("WHEEE", col3, 0.2f);


        float size[2] = {};
        ct_debugui_a0.GetWindowSize(size);
        size[1] = size[0];
        ct_debugui_a0.Image2(ct_texture_a0.get(ct_hash_a0.id64_from_str("content/scene/m4a1/m4_diff")),
                             size,
                             (float[2]) {0.0f, 0.0f},
                             (float[2]) {1.0f, 1.0f},
                             (float[4]) {1.0f, 1.0f, 1.0f, 1.0f},
                             (float[4]) {0.0f, 0.0f, 0.0, 0.0f});
    }
    ct_debugui_a0.EndDock();

}

void module2() {
    static bool visible = true;
    if (ct_debugui_a0.BeginDock("Module 2", &visible,
                                DebugUIWindowFlags_Empty)) {
        ct_debugui_a0.Text("dear imgui, %d", 111);
        ct_debugui_a0.Text("By Omar Cornut and all github contributors.");
        ct_debugui_a0.Text(
                "ImGui is licensed under the MIT License, see LICENSE for more information.");
    }
    ct_debugui_a0.EndDock();
}



//==============================================================================
// Module def
//==============================================================================
CETECH_MODULE_DEF(
        example,

//==============================================================================
// Init api
//==============================================================================
        {
            CETECH_GET_API(api, ct_keyboard_a0);
            CETECH_GET_API(api, ct_log_a0);
            CETECH_GET_API(api, ct_app_a0);
            CETECH_GET_API(api, ct_playground_a0);
            CETECH_GET_API(api, ct_debugui_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_renderer_a0);


            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_world_a0);
            CETECH_GET_API(api, ct_entity_a0);
            CETECH_GET_API(api, ct_camera_a0);
            CETECH_GET_API(api, ct_level_a0);
            CETECH_GET_API(api, ct_texture_a0);
        },

//==============================================================================
// Load
//==============================================================================
        {
            CEL_UNUSED(api);

            ct_log_a0.info("example", "Init %d", reload);

            ct_app_a0.register_on_update(update);

            //ct_debugui_a0.register_on_debugui(module1);
            //ct_debugui_a0.register_on_debugui(module2);
        },

//==============================================================================
// Unload
//==============================================================================
        {
            CEL_UNUSED(api);

            ct_log_a0.info("example", "Shutdown %d", reload);

            ct_app_a0.unregister_on_update(update);

            ct_debugui_a0.unregister_on_debugui(module1);
            ct_debugui_a0.unregister_on_debugui(module2);
        }
)


