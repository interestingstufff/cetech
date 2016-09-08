//==============================================================================
// includes
//==============================================================================

#include <celib/stringid/types.h>
#include "../renderer.h"
#include "bgfx/c99/bgfxplatform.h"
#include "celib/window/window.h"
#include "bgfx_texture_resource.h"

//==============================================================================
// GLobals
//==============================================================================

#define _G RendererGlobals
static struct G {
    stringid64_t texture_type;
    u32 size_width;
    u32 size_height;
    int capture;
    int vsync;
    int need_reset;
} _G = {0};


//==============================================================================
// Private
//==============================================================================

static u32 _get_reset_flags() {
    return (_G.capture ? BGFX_RESET_CAPTURE : 0) |
           (_G.vsync ? BGFX_RESET_VSYNC : 0);
}

//==============================================================================
// Interface
//==============================================================================

int renderer_init() {
    _G = (struct G) {0};


    texture_resource_init();

    return 1;
}

void renderer_shutdown() {
    texture_resource_shutdown();

    bgfx_shutdown();

    _G = (struct G) {0};
}

void renderer_create(window_t window) {
    bgfx_platform_data_t pd = {0};
    pd.nwh = window_native_window_ptr(window);
    pd.ndt = window_native_display_ptr(window);
    bgfx_set_platform_data(&pd);

    // TODO: from config
    bgfx_init(BGFX_RENDERER_TYPE_OPENGL, 0, 0, NULL, NULL);

    window_get_size(window, &_G.size_width, &_G.size_height);

    _G.need_reset = 1;
}

void renderer_set_debug(int debug) {
    if (debug) {
        bgfx_set_debug(BGFX_DEBUG_STATS);
    } else {
        bgfx_set_debug(BGFX_DEBUG_NONE);
    }
}

void renderer_render_world(world_t world, camera_t camera, viewport_t viewport) {
    if (CE_UNLIKELY(_G.need_reset)) {
        bgfx_reset(_G.size_width, _G.size_height, _get_reset_flags());
    }

    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x66CCFFff, 1.0f, 0);

    bgfx_set_view_rect(0, 0, 0, (uint16_t) _G.size_width, (uint16_t) _G.size_height);

    bgfx_touch(0);
    bgfx_dbg_text_clear(0, 0);

    bgfx_frame(0);
}
