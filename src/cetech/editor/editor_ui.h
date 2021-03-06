#ifndef CETECH_RESOURCE_UI_H
#define CETECH_RESOURCE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#define CT_RESOURCE_UI_API \
    CE_ID64_0("ct_editor_ui_a0", 0x864e3516a3c025aeULL)

#define CT_LOCKED_OBJ \
    CE_ID64_0("locked_obj", 0xa0bd856b4fcd25aeULL)

typedef struct ui_vec4_p0 {
    float min_f;
    float max_f;
    bool color;
} ui_vec4_p0;

typedef struct ui_vec3_p0 {
    float min_f;
    float max_f;
    bool color;
} ui_vec3_p0;

typedef struct ui_float_p0 {
    float min_f;
    float max_f;
} ui_float_p0;

struct ct_editor_ui_a0 {
    void (*prop_float)(uint64_t obj,
                       uint64_t property,
                       const char *label,
                       ui_float_p0 params);

    void (*prop_bool)(uint64_t obj,
                      uint64_t property,
                      const char *label);

    void (*prop_str)(uint64_t obj,
                     uint64_t property,
                     const char *label,
                     uint32_t i);

    void (*prop_str_combo)(uint64_t obj,
                           uint64_t property,
                           const char *label,
                           void (*combo_items)(uint64_t obj,
                                               char **items,
                                               uint32_t *items_count),
                           uint32_t i);

    void (*prop_resource)(uint64_t obj,
                          uint64_t property,
                          const char *label,
                          uint64_t resource_type,
                          uint64_t context,
                          uint32_t i);

    void (*prop_vec3)(uint64_t obj,
                      const uint64_t property[3],
                      const char *label,
                      ui_vec3_p0 params);

    void (*prop_vec4)(uint64_t obj,
                      const uint64_t property[4],
                      const char *label,
                      ui_vec4_p0 params);


    bool (*prop_revert_btn)(uint64_t _obj,
                            const uint64_t *props,
                            uint64_t props_n);

    void (*resource_tooltip)(ct_resource_id_t0 resourceid,
                             const char *path,
                             ce_vec2_t size);

    bool (*resource_select_modal)(const char *modal_id,
                                  uint64_t id,
                                  uint64_t resource_type,
                                  uint64_t *selected_resource,
                                  uint32_t *count);

    uint64_t (*lock_selected_obj)(uint64_t dock,
                                  uint64_t selected_obj);

    void (*ui_prop_header)(const char *name);

    void (*begin_disabled)();
    void (*end_disabled)();

};

CE_MODULE(ct_editor_ui_a0);

#ifdef __cplusplus
};
#endif

#endif //CETECH_RESOURCE_UI_H
