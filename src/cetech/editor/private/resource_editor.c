#include <stdio.h>
#include <string.h>

#include <celib/memory/allocator.h>
#include <celib/cdb.h>
#include <celib/ydb.h>
#include <celib/math/math.h>
#include <celib/macros.h>
#include "celib/id.h"
#include "celib/memory/memory.h"
#include "celib/api.h"
#include "celib/module.h"


#include <cetech/ecs/ecs.h>
#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/debugui/debugui.h>
#include <cetech/camera/camera.h>
#include <cetech/transform/transform.h>
#include <cetech/controlers/keyboard.h>
#include <cetech/editor/resource_browser.h>
#include <cetech/editor/explorer.h>
#include <cetech/editor/editor.h>
#include <cetech/resource/resource.h>
#include <cetech/render_graph/render_graph.h>
#include <cetech/default_rg/default_rg.h>
#include <cetech/debugui/icons_font_awesome.h>
#include <cetech/debugui/debugui.h>
#include <cetech/editor/dock.h>
#include <cetech/controlers/controlers.h>
#include <celib/containers/array.h>
#include <cetech/editor/resource_editor.h>
#include <cetech/editor/selcted_object.h>
#include <cetech/editor/dock.h>

#define _G editor_globals

#define _PROP_EDITOR\
    CE_ID64_0("editor", 0xf76c66a1ef2e2f59ULL)

#define _ASSET_EDITOR\
    CE_ID64_0("asset_editor", 0xd7ecca607e454ce9ULL)


typedef struct editor {
    char title[128];
    uint64_t type;
    uint64_t obj;
    uint64_t context_obj;
}editor;

static struct _G {
    struct editor *editors;
} _G;


static struct ct_resource_editor_i0 *get_asset_editor(uint64_t cdb_type) {
    struct ce_api_entry_t0 it = ce_api_a0->first(RESOURCE_EDITOR_I);
    while (it.api) {
        struct ct_resource_editor_i0 *i = (it.api);

        if (cdb_type == i->cdb_type()) {
            return i;
        }

        it = ce_api_a0->next(it);
    }

    return NULL;
};

static void draw_editor(uint64_t dock) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(),dock);

    struct editor *editor = ce_cdb_a0->read_ptr(reader, _PROP_EDITOR, NULL);

    if(!editor) {
        return;
    }

    struct ct_resource_editor_i0 *i = get_asset_editor(editor->type);

    if (!i) {
        return;
    }

    const uint64_t context = ce_cdb_a0->read_uint64(reader, PROP_DOCK_CONTEXT, 0);

    bool is_mouse_hovering = ct_debugui_a0->IsMouseHoveringWindow();
    bool click = ct_debugui_a0->IsMouseClicked(0, false);

    if (is_mouse_hovering && click) {
        const ce_cdb_obj_o0 *creader = ce_cdb_a0->read(ce_cdb_a0->db(),editor->context_obj);
        uint64_t name = ce_cdb_a0->read_uint64(creader,
                                               RESOURCE_EDITOR_OBJ, 0);

        uint64_t obj = name;

        ct_selected_object_a0->set_selected_object(context, obj);
    }

    i->draw_ui(editor->context_obj, context);
}

static void draw_editor_menu(uint64_t dock) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(),dock);

    struct editor *editor = ce_cdb_a0->read_ptr(reader, _PROP_EDITOR, NULL);

    if(!editor) {
        return;
    }

    struct ct_resource_editor_i0 *i = get_asset_editor(editor->type);

    if (!i) {
        return;
    }

    ct_dock_a0->context_btn(dock);
    if(i && i->draw_menu) {
        ct_debugui_a0->SameLine(0, -1);
        i->draw_menu(editor->context_obj);
    }
}

static uint32_t find_editor(uint64_t obj) {
    const uint32_t editor_n = ce_array_size(_G.editors);

    for (uint32_t i = 0; i < editor_n; ++i) {
        struct editor *editor = &_G.editors[i];

        if (editor->obj == obj) {
            return i;
        }
    }

    return UINT32_MAX;
}

#define DEFAULT_EDITOR_NAME  "Editor"

static const char *dock_title(uint64_t dock) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(),dock);
    struct editor *editor = ce_cdb_a0->read_ptr(reader, _PROP_EDITOR, NULL);

    if (!editor) {
        return NULL;
    }

    struct ct_resource_editor_i0 *i = get_asset_editor(editor->type);

    if (!i) {
        return DEFAULT_EDITOR_NAME;
    }


    const char *display_icon = i->display_icon ? i->display_icon() : "";
    const char *display_name = i->display_name ? i->display_name()
                                               : DEFAULT_EDITOR_NAME;

    snprintf(editor->title, CE_ARRAY_LEN(editor->title),
             "%s %s", display_icon, display_name);

    return editor->title;
}

static const char *name(uint64_t dock) {
    return "editor";
}

static struct editor *_get_or_create_editor(uint64_t obj) {
    uint32_t editor_idx = find_editor(obj);

    if (editor_idx != UINT32_MAX) {
        struct editor *editor = &_G.editors[editor_idx];
        return editor;
    }

    int idx = ce_array_size(_G.editors);
    ce_array_push(_G.editors, (editor) {}, ce_memory_a0->system);

    struct editor *editor = &_G.editors[idx];

    editor->context_obj = ce_cdb_a0->create_object(ce_cdb_a0->db(), 0);

    uint64_t dock = ct_dock_a0->create_dock(_ASSET_EDITOR, true);

    ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(),dock);
    ce_cdb_a0->set_ptr(w, _PROP_EDITOR, editor);
    ce_cdb_a0->write_commit(w);

    return editor;
}

static void open(uint64_t obj) {
    uint64_t type = ce_cdb_a0->obj_type(ce_cdb_a0->db(), obj);
    struct ct_resource_editor_i0 *i = get_asset_editor(type);

    if (!i) {
        return;
    }

    struct editor *e = _get_or_create_editor(obj);
    e->type = type;
    e->obj = obj;

    ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(),e->context_obj);
    ce_cdb_a0->set_ref(w, RESOURCE_EDITOR_OBJ, obj);
    ce_cdb_a0->write_commit(w);

    i->open(e->context_obj);

}

static void update(float dt) {
    uint64_t edited = ct_resource_browser_a0->edited();
    if (edited) {
        open(edited);
    }

    const uint32_t editor_n = ce_array_size(_G.editors);
    for (uint8_t i = 0; i < editor_n; ++i) {
        struct editor *editor = &_G.editors[i];

        struct ct_resource_editor_i0 *editor_i = get_asset_editor(editor->type);

        if (!editor_i) {
            continue;
        }

        editor_i->update(editor->context_obj, dt);
    }
}


static struct ct_editor_module_i0 editor_module_api = {
        .update = update,
};

static uint64_t cdb_type() {
    return _ASSET_EDITOR;
};

static uint64_t dock_flags() {
    return DebugUIWindowFlags_NoNavInputs |
           DebugUIWindowFlags_NoScrollbar |
           DebugUIWindowFlags_NoScrollWithMouse;
}


static struct ct_dock_i0 dock_i = {
        .cdb_type = cdb_type,
        .dock_flags = dock_flags,
        .display_title = dock_title,
        .name = name,
        .draw_ui = draw_editor,
        .draw_menu = draw_editor_menu,
};

static void _init(struct ce_api_a0 *api) {
    _G = (struct _G) {
    };

    ce_api_a0->register_api(DOCK_INTERFACE, &dock_i, sizeof(dock_i));
    ce_api_a0->register_api(EDITOR_MODULE_INTERFACE, &editor_module_api, sizeof(editor_module_api));
}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        editor,
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
