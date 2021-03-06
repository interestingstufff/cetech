#include <float.h>
#include <stdio.h>
#include <time.h>

#include <celib/macros.h>
#include <celib/memory/allocator.h>
#include <celib/id.h>
#include <celib/config.h>
#include <celib/memory/memory.h>
#include <celib/api.h>
#include <celib/ydb.h>
#include <celib/containers/array.h>
#include <celib/module.h>


#include <celib/containers/hash.h>
#include <celib/cdb.h>

#include <celib/math/math.h>
#include <cetech/editor/resource_browser.h>
#include <cetech/resource/resourcedb.h>

#include <cetech/editor/property.h>
#include <cetech/resource/resource.h>
#include <cetech/debugui/icons_font_awesome.h>
#include <stdlib.h>
#include <cetech/editor/resource_preview.h>
#include <cetech/controlers/controlers.h>
#include <cetech/controlers/keyboard.h>

#include <cetech/renderer/gfx.h>
#include <cetech/debugui/debugui.h>
#include <cetech/editor/selcted_object.h>

#include "cetech/editor/editor_ui.h"

static bool prop_revert_btn(uint64_t _obj,
                            const uint64_t *props,
                            uint64_t props_n) {
    const ce_cdb_obj_o0 *r = ce_cdb_a0->read(ce_cdb_a0->db(), _obj);
    uint64_t instance_of = ce_cdb_a0->read_instance_of(r);


    bool need_revert = false;
    if (instance_of) {
        const ce_cdb_obj_o0 *ir = ce_cdb_a0->read(ce_cdb_a0->db(), instance_of);

        for (int i = 0; i < props_n; ++i) {
            if (!ce_cdb_a0->prop_equal(r, ir, props[i])) {
                need_revert = true;
                break;
            }
        }
    }

    char lbl[256] = {};
    snprintf(lbl, CE_ARRAY_LEN(lbl), "%s##revert_%llu_%llu",
             ICON_FA_RECYCLE, _obj, props[0]);

    if (!need_revert) {
        ct_debugui_a0->PushItemFlag(DebugUIItemFlags_Disabled, true);
        ct_debugui_a0->PushStyleVar(DebugUIStyleVar_Alpha, 0.5f);
    }

    bool remove_change = ct_debugui_a0->Button(lbl, &(ce_vec2_t) {});

    if (!need_revert) {
        ct_debugui_a0->PopItemFlag();
        ct_debugui_a0->PopStyleVar(1);
    }

    if (need_revert && ct_debugui_a0->IsItemHovered(0)) {
        const ce_cdb_obj_o0 *ir = ce_cdb_a0->read(ce_cdb_a0->db(), instance_of);

        char v_str[256] = {};
        uint32_t offset = 0;

        for (int i = 0; i < props_n; ++i) {
            uint64_t prop = props[i];
            enum ce_cdb_type_e0 ptype = ce_cdb_a0->prop_type(ir, prop);

            switch (ptype) {

                case CDB_TYPE_NONE:
                    break;

                case CDB_TYPE_UINT64: {
                    uint64_t u = ce_cdb_a0->read_uint64(ir,
                                                        prop,
                                                        0);

                    offset += snprintf(v_str + offset,
                                       CE_ARRAY_LEN(v_str) - offset,
                                       "%llu ", u);
                }
                    break;

                case CDB_TYPE_PTR:
                    break;

                case CDB_TYPE_REF: {
                    uint64_t ref = ce_cdb_a0->read_ref(ir, prop, 0);
                    if (!ref) {
                        break;
                    }

                    const ce_cdb_obj_o0 *rr = \
                            ce_cdb_a0->read(ce_cdb_a0->db(), ref);

                    const char *res_name = ce_cdb_a0->read_str(rr,
                                                               ASSET_NAME_PROP,
                                                               "");
                    if (res_name) {
                        offset += snprintf(v_str + offset,
                                           CE_ARRAY_LEN(v_str) - offset,
                                           "%s ", res_name);
                    }
                }
                    break;

                case CDB_TYPE_FLOAT: {
                    float f = ce_cdb_a0->read_float(ir,
                                                    prop,
                                                    0);

                    offset += snprintf(v_str + offset,
                                       CE_ARRAY_LEN(v_str) - offset,
                                       "%f ", f);
                }
                    break;
                case CDB_TYPE_BOOL:
                    break;

                case CDB_TYPE_STR: {
                    const char *str = ce_cdb_a0->read_str(ir,
                                                          prop,
                                                          NULL);

                    offset += snprintf(v_str + offset,
                                       CE_ARRAY_LEN(v_str) - offset,
                                       "%s ", str);
                }


                    break;
                case CDB_TYPE_SUBOBJECT:
                    break;

                case CDB_TYPE_BLOB:
                    break;
            }
        }

        if (v_str[0]) {
            ct_debugui_a0->BeginTooltip();
            ct_debugui_a0->Text("Parent value: %s", v_str);
            ct_debugui_a0->EndTooltip();
        }

    }

    if (need_revert && remove_change) {
        const ce_cdb_obj_o0 *ir = ce_cdb_a0->read(ce_cdb_a0->db(), instance_of);
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), _obj);
        for (int i = 0; i < props_n; ++i) {
            ce_cdb_a0->prop_copy(ir, w, props[i]);
        }
        ce_cdb_a0->write_commit(w);
    }

    return true;
}


static void _prop_label(const char *label,
                        uint64_t obj,
                        const uint64_t *props,
                        uint64_t props_n) {

    prop_revert_btn(obj, props, props_n);
    ct_debugui_a0->SameLine(0, 8);
    ct_debugui_a0->Text("%s", label);
}


static void resource_tooltip(ct_resource_id_t0 resourceid,
                             const char *path,
                             ce_vec2_t size) {
    ct_debugui_a0->Text("%s", path);

    uint64_t type = ct_resourcedb_a0->get_resource_type(resourceid);

    struct ct_resource_i0 *ri = ct_resource_a0->get_interface(type);

    if (!ri || !ri->get_interface) {
        return;
    }

    struct ct_resource_preview_i0 *ai = (ri->get_interface(RESOURCE_PREVIEW_I));

    uint64_t obj = resourceid.uid;

    if (ai) {
        if (ai->tooltip) {
            ai->tooltip(obj, size);
        }

        ct_resource_preview_a0->set_background_resource(resourceid);
        ct_resource_preview_a0->draw_background_texture(size);
    }

}

static void ui_float(uint64_t obj,
                     uint64_t prop,
                     const char *label,
                     struct ui_float_p0 params) {
    float value = 0;
    float value_new = 0;

    if (!obj) {
        return;
    }

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);

    value_new = ce_cdb_a0->read_float(reader, prop, value_new);
    value = value_new;

    const float min = !params.max_f ? -FLT_MAX : params.min_f;
    const float max = !params.max_f ? FLT_MAX : params.max_f;

    _prop_label(label, obj, &prop, 1);

    char labelid[128] = {'\0'};
    sprintf(labelid, "##%sprop_float_%d", label, 0);

    if (ct_debugui_a0->DragFloat(labelid,
                                 &value_new, 1.0f,
                                 min, max,
                                 "%.3f", 1.0f)) {

        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_float(w, prop, value_new);
        ce_cdb_a0->write_commit(w);
    }

}

static void ui_bool(uint64_t obj,
                    uint64_t prop,
                    const char *label) {
    bool value = false;
    bool value_new = false;

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);

    value_new = ce_cdb_a0->read_bool(reader, prop, value_new);
    value = value_new;

    _prop_label(label, obj, &prop, 1);

    char labelid[128] = {'\0'};
    sprintf(labelid, "##%sprop_float_%d", label, 0);

    ct_debugui_a0->SameLine(0, 2);

    if (ct_debugui_a0->Checkbox(labelid, &value_new)) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_bool(w, prop, value_new);
        ce_cdb_a0->write_commit(w);
    }
}


static void ui_str(uint64_t obj,
                   uint64_t prop,
                   const char *label,
                   uint32_t i) {
    char labelid[128] = {'\0'};

    const char *value = 0;

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);
    value = ce_cdb_a0->read_str(reader, prop, "");

    char buffer[128] = {'\0'};
    strcpy(buffer, value);

    sprintf(labelid, "##%sprop_str_%d", label, i);

    _prop_label(label, obj, &prop, 1);

    bool change = false;

    ct_debugui_a0->PushItemWidth(-1);
    change |= ct_debugui_a0->InputText(labelid,
                                       buffer,
                                       CE_ARRAY_LEN(buffer),
                                       0,
                                       0, NULL);
    ct_debugui_a0->PopItemWidth();

    if (change) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_str(w, prop, buffer);
        ce_cdb_a0->write_commit(w);
    }
}

static void ui_str_combo(uint64_t obj,
                         uint64_t prop,
                         const char *label,
                         void (*combo_items)(uint64_t obj,
                                             char **items,
                                             uint32_t *items_count),
                         uint32_t i) {

    const char *value = 0;

    if (!obj) {
        return;
    }

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);
    value = ce_cdb_a0->read_str(reader, prop, NULL);

    char *items = NULL;
    uint32_t items_count = 0;

    combo_items(obj, &items, &items_count);

    int current_item = -1;

    const char *items2[items_count];
    memset(items2, 0, sizeof(const char *) * items_count);

    for (int j = 0; j < items_count; ++j) {
        items2[j] = &items[j * 128];
        if (value) {
            if (ce_id_a0->id64(items2[j]) == ce_id_a0->id64(value)) {
                current_item = j;
            }
        }
    }

    _prop_label(label, obj, &prop, 1);

    char labelid[128] = {'\0'};

    char buffer[128] = {'\0'};

    if (value) {
        strcpy(buffer, value);
    }

    sprintf(labelid, "##%scombo_%d", label, i);
    ct_debugui_a0->PushItemWidth(-1);
    bool change = ct_debugui_a0->Combo(labelid,
                                       &current_item, items2,
                                       items_count, -1);
    ct_debugui_a0->PopItemWidth();
    if (change) {
        strcpy(buffer, items2[current_item]);
    }

    if (change) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_str(w, prop, buffer);
        ce_cdb_a0->write_commit(w);
    }

}


static char modal_buffer[128] = {};

static bool resource_select_modal(const char *modal_id,
                                  uint64_t id,
                                  uint64_t resource_type,
                                  uint64_t *selected_resource,
                                  uint32_t *count) {
    bool changed = false;
    bool open = true;

    ct_debugui_a0->SetNextWindowSize(&(ce_vec2_t) {512, 512}, 0);
    if (ct_debugui_a0->BeginPopupModal(modal_id, &open, 0)) {
        struct ct_controlers_i0 *kb = ct_controlers_a0->get(CONTROLER_KEYBOARD);

        if (kb->button_pressed(0, kb->button_index("escape"))) {
            ct_debugui_a0->CloseCurrentPopup();
            ct_debugui_a0->EndPopup();
            return false;
        }

        char labelidi[128] = {'\0'};
        sprintf(labelidi, "##modal_input%llu", id);

        ct_debugui_a0->InputText(labelidi,
                                 modal_buffer,
                                 CE_ARRAY_LEN(modal_buffer),
                                 0,
                                 0, NULL);

        if (count) {
            int c = *count;
            ct_debugui_a0->InputInt("Count", &c, 1, 1, 0);
            *count = (uint32_t) c;
        }

        const char *resource_type_s = ce_id_a0->str_from_id64(resource_type);
        char **resources = NULL;

        ct_resourcedb_a0->get_resource_by_type(modal_buffer, resource_type_s,
                                               &resources,
                                               ce_memory_a0->system);

        uint32_t dir_n = ce_array_size(resources);
        for (int i = 0; i < dir_n; ++i) {
            const char *name = resources[i];

            if (!strlen(name)) {
                continue;
            }

            bool selected = ct_debugui_a0->Selectable(name, false, 0,
                                                      &CE_VEC2_ZERO);

            if (ct_debugui_a0->IsItemHovered(0)) {
                struct ct_resource_id_t0 r = {
                        .uid=ct_resourcedb_a0->get_uid(name, resource_type_s)
                };

                ct_debugui_a0->BeginTooltip();
                ct_editor_ui_a0->resource_tooltip(r, name,
                                                  (ce_vec2_t) {256, 256});
                ct_debugui_a0->EndTooltip();
            }

            if (selected) {
                *selected_resource = ct_resourcedb_a0->get_uid(name,
                                                               resource_type_s);
                changed = true;

                modal_buffer[0] = '\0';
                break;
            }
        }

        ct_resourcedb_a0->get_resource_by_type_clean(resources,
                                                     ce_memory_a0->system);
        ct_debugui_a0->EndPopup();
    }

    return changed;
}

void ui_prop_tree_node(const char *name) {
    ct_debugui_a0->Separator();
    ct_debugui_a0->Text("%s", name);
}

static void ui_resource(uint64_t obj,
                        uint64_t prop,
                        const char *label,
                        uint64_t resource_type,
                        uint64_t context,
                        uint32_t i) {
    if (!obj) {
        return;
    }

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);

    uint64_t uid = ce_cdb_a0->read_ref(reader, prop, 0);

    uint64_t resource_obj = uid;

    const ce_cdb_obj_o0 *r = ce_cdb_a0->read(ce_cdb_a0->db(), resource_obj);
    const char *resource_name = ce_cdb_a0->read_str(r, ASSET_NAME_PROP, "");

    char buffer[128] = {'\0'};
    sprintf(buffer, "%s", resource_name);

    char labelid[128] = {'\0'};
    char modal_id[128] = {'\0'};

    bool change = false;

    struct ct_resource_i0 *ri = ct_resource_a0->get_interface(resource_type);


    uint64_t new_value = 0;

    sprintf(modal_id, ICON_FA_FOLDER_OPEN
            " ""select...##select_resource_%d", i);

    change = resource_select_modal(modal_id, obj + prop,
                                   resource_type, &new_value, NULL);


    const char *icon = ri->display_icon ? ri->display_icon() : NULL;
    if (icon) {
        snprintf(labelid, CE_ARRAY_LEN(labelid), "%s %s", icon, label);
    } else {
        snprintf(labelid, CE_ARRAY_LEN(labelid), "%s", label);
    }

    _prop_label(labelid, obj, &prop, 1);

    sprintf(labelid, ICON_FA_ARROW_UP "##%sprop_open_select_resource_%d", label, i);

    if (ct_debugui_a0->Button(labelid, &(ce_vec2_t) {0.0f})) {
        ct_selected_object_a0->set_selected_object(context, uid);
    };

    ct_debugui_a0->SameLine(0, 2);

    // Open btn
    sprintf(labelid, ICON_FA_FOLDER_OPEN "##%sprop_select_resource_%d", label, i);

    if (ct_debugui_a0->Button(labelid, &(ce_vec2_t) {0.0f})) {
        ct_debugui_a0->OpenPopup(modal_id);
    };
    ct_debugui_a0->SameLine(0, 2);

    ct_debugui_a0->PushItemWidth(-1);
    sprintf(labelid, "##%sresource_prop_str_%d", label, i);

    ct_debugui_a0->InputText(labelid,
                             buffer,
                             strlen(buffer),
                             DebugInputTextFlags_ReadOnly,
                             0, NULL);
    ct_debugui_a0->PopItemWidth();

    if (ct_debugui_a0->BeginDragDropTarget()) {
        const struct DebugUIPayload *payload;
        payload = ct_debugui_a0->AcceptDragDropPayload("asset", 0);

        if (payload) {
            uint64_t drag_obj = *((uint64_t *) payload->Data);

            if (drag_obj) {

                uint64_t asset_type = ce_cdb_a0->obj_type(ce_cdb_a0->db(),
                                                          drag_obj);

                if (resource_type == asset_type) {
                    new_value = drag_obj;
                    change = true;
                }
            }
        }

        ct_debugui_a0->EndDragDropTarget();
    }

    if (change) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_ref(w, prop, new_value);
        ce_cdb_a0->write_commit(w);
    }

}

static void ui_vec3(uint64_t obj,
                    const uint64_t prop[3],
                    const char *label,
                    struct ui_vec3_p0 params) {
    if (!obj) {
        return;
    }

    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);

    ce_vec3_t value = {
            .x = ce_cdb_a0->read_float(reader, prop[0], 0.0f),
            .y = ce_cdb_a0->read_float(reader, prop[1], 0.0f),
            .z = ce_cdb_a0->read_float(reader, prop[2], 0.0f),
    };

    ce_vec3_t value_new = value;

    const float min = !params.min_f ? -FLT_MAX : params.min_f;
    const float max = !params.max_f ? FLT_MAX : params.max_f;

    _prop_label(label, obj, prop, 3);

    char labelid[128] = {'\0'};
    sprintf(labelid, "##%sprop_vec3_%d", label, 0);

    ct_debugui_a0->PushItemWidth(-1);
    if (ct_debugui_a0->DragFloat3(labelid,
                                  (float *) &value_new, 1.0f,
                                  min, max,
                                  "%.3f", 1.0f)) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_float(w, prop[0], value_new.x);
        ce_cdb_a0->set_float(w, prop[1], value_new.y);
        ce_cdb_a0->set_float(w, prop[2], value_new.z);
        ce_cdb_a0->write_commit(w);
    }

    ct_debugui_a0->PopItemWidth();
}

static void ui_vec4(uint64_t obj,
                    const uint64_t prop[4],
                    const char *label,
                    struct ui_vec4_p0 params) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), obj);

    ce_vec4_t value = {
            .x = ce_cdb_a0->read_float(reader, prop[0], 0.0f),
            .y = ce_cdb_a0->read_float(reader, prop[1], 0.0f),
            .z = ce_cdb_a0->read_float(reader, prop[2], 0.0f),
            .w = ce_cdb_a0->read_float(reader, prop[3], 0.0f),
    };

    ce_vec4_t value_new = value;

    const float min = !params.min_f ? -FLT_MAX : params.min_f;
    const float max = !params.max_f ? FLT_MAX : params.max_f;

    _prop_label(label, obj, prop, 4);

    char labelid[128] = {'\0'};
    sprintf(labelid, "##%sprop_vec3_%d", label, 0);

    ct_debugui_a0->PushItemWidth(-1);

    bool changed;
    if (params.color) {
        changed = ct_debugui_a0->ColorEdit4(labelid,
                                            (float *) &value_new, 1);
    } else {
        changed = ct_debugui_a0->DragFloat4(labelid,
                                            (float *) &value_new, 1.0f,
                                            min, max,
                                            "%.3f", 1.0f);
    }

    if (changed) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), obj);
        ce_cdb_a0->set_float(w, prop[0], value_new.x);
        ce_cdb_a0->set_float(w, prop[1], value_new.y);
        ce_cdb_a0->set_float(w, prop[2], value_new.z);
        ce_cdb_a0->set_float(w, prop[3], value_new.w);
        ce_cdb_a0->write_commit(w);
    }
    ct_debugui_a0->PopItemWidth();
}

static uint64_t lock_selected_obj(uint64_t dock,
                                  uint64_t selected_obj) {
    const ce_cdb_obj_o0 *reader = ce_cdb_a0->read(ce_cdb_a0->db(), dock);

    uint64_t locked_object = ce_cdb_a0->read_ref(reader, CT_LOCKED_OBJ, 0);
    bool checked = locked_object != 0;
    if (ct_debugui_a0->Checkbox(ICON_FA_LOCK, &checked)) {
        ce_cdb_obj_o0 *w = ce_cdb_a0->write_begin(ce_cdb_a0->db(), dock);
        if (checked) {
            ce_cdb_a0->set_ref(w, CT_LOCKED_OBJ, selected_obj);
            locked_object = selected_obj;
        } else {
            ce_cdb_a0->remove_property(w, CT_LOCKED_OBJ);
        }
        ce_cdb_a0->write_commit(w);
    }

    return locked_object;
}

void begin_disabled() {
    ct_debugui_a0->PushItemFlag(DebugUIItemFlags_Disabled, true);
    ct_debugui_a0->PushStyleVar(DebugUIStyleVar_Alpha, 0.5f);
}

void end_disabled() {
    ct_debugui_a0->PopItemFlag();
    ct_debugui_a0->PopStyleVar(1);
}

static struct ct_editor_ui_a0 editor_ui_a0 = {
        .prop_float = ui_float,
        .prop_str = ui_str,
        .prop_str_combo = ui_str_combo,
        .prop_resource = ui_resource,
        .prop_vec3 = ui_vec3,
        .prop_vec4 = ui_vec4,
        .prop_bool = ui_bool,
        .prop_revert_btn = prop_revert_btn,
        .resource_tooltip = resource_tooltip,
        .resource_select_modal = resource_select_modal,
        .lock_selected_obj = lock_selected_obj,
        .ui_prop_header = ui_prop_tree_node,
        .begin_disabled = begin_disabled,
        .end_disabled = end_disabled,
};

struct ct_editor_ui_a0 *ct_editor_ui_a0 = &editor_ui_a0;

static void _init(struct ce_api_a0 *api) {
    api->register_api(CT_RESOURCE_UI_API, ct_editor_ui_a0, sizeof(editor_ui_a0));
}

static void _shutdown() {
}

CE_MODULE_DEF(
        sourcedb_ui,
        {
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ce_id_a0);
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
