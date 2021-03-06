#include <string.h>


#include <celib/macros.h>
#include "celib/id.h"
#include "celib/memory/memory.h"
#include "celib/module.h"

#include "celib/api.h"


#include <cetech/debugui/icons_font_awesome.h>
#include <cetech/kernel/kernel.h>
#include <cetech/renderer/renderer.h>
#include <cetech/editor/dock.h>
#include <cetech/renderer/gfx.h>
#include <cetech/game/game_system.h>

#include <cetech/debugui/debugui.h>

#define WINDOW_NAME "Game view"

static float step_dt = 16.0f;

static void on_menu(uint64_t dock) {
    uint64_t game_name = ce_id_a0->id64("default");

    static const char *label[] = {
            ICON_FA_PAUSE,
            ICON_FA_PLAY,
    };

    bool is_paused = ct_game_system_a0->is_paused(game_name);

    if (ct_debugui_a0->Button(label[is_paused], &CE_VEC2_ZERO)) {
        if (is_paused) {
            ct_game_system_a0->play(game_name);
        } else {
            ct_game_system_a0->pause(game_name);
        }
    }

    if (is_paused) {
        ct_debugui_a0->SameLine(0.0f, 4.0f);
        if (ct_debugui_a0->Button(ICON_FA_FORWARD, &CE_VEC2_ZERO)) {
            ct_game_system_a0->step(game_name, step_dt / 1000.0f);
        }

        ct_debugui_a0->SameLine(0.0f, 4.0f);
        ct_debugui_a0->InputFloat("delta", &step_dt, 0.0f, 0.0f, -1, 0);
    }

}

static void on_debugui(uint64_t dock) {
    ce_vec2_t size = ct_debugui_a0->GetContentRegionAvail();

    uint64_t game_name = ce_id_a0->id64("default");

    struct ct_viewport_t0 v = ct_game_system_a0->viewport(game_name);
    struct ct_rg_builder_t0 *builder;
    builder = ct_renderer_a0->viewport_builder(v);

    builder->set_size(builder, size.x, size.y);


    bgfx_texture_handle_t th;
    th = builder->get_texture(builder, RG_OUTPUT_TEXTURE);

    ct_debugui_a0->Image(th,
                         &size,
                         &CE_VEC4_ONE,
                         &CE_VEC4_ZERO);

}

static const char *dock_title(uint64_t dock) {
    return ICON_FA_GAMEPAD " " WINDOW_NAME;
}

static const char *name(uint64_t dock) {
    return "game_view";
}


static uint64_t cdb_type() {
    return GAME_INTERFACE;
};


static uint64_t dock_flags() {
    return DebugUIWindowFlags_NoNavInputs |
           DebugUIWindowFlags_NoScrollbar |
           DebugUIWindowFlags_NoScrollWithMouse;
}

static struct ct_dock_i0 dock_api = {
        .dock_flags = dock_flags,
        .cdb_type = cdb_type,
        .name = name,
        .display_title = dock_title,
        .draw_ui = on_debugui,
        .draw_menu = on_menu,
};

static void _init(struct ce_api_a0 *api) {
    api->register_api(DOCK_INTERFACE, &dock_api, sizeof(dock_api));

    ct_dock_a0->create_dock(GAME_INTERFACE, true);
}

static void _shutdown() {
}

CE_MODULE_DEF(
        game_view,
        {
            CE_INIT_API(api, ce_id_a0);
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