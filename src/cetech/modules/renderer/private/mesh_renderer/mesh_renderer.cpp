#include "celib/array.inl"
#include "celib/map.inl"

#include "cetech/kernel/hashlib.h"
#include "cetech/kernel/config.h"
#include "cetech/kernel/memory.h"
#include "cetech/kernel/api_system.h"

#include "cetech/kernel/resource.h"

#include "cetech/modules/entity/entity.h"

#include "cetech/modules/transform/transform.h"
#include "cetech/modules/scenegraph/scenegraph.h"

#include <bgfx/bgfx.h>
#include <cetech/kernel/macros.h>
#include <cetech/kernel/blob.h>
#include <cetech/modules/renderer/renderer.h>
#include <cetech/modules/renderer/scene.h>
#include <cetech/modules/renderer/material.h>
#include <cetech/kernel/module.h>
#include <cetech/modules/renderer/mesh_renderer.h>
#include <cetech/kernel/ydb.h>
#include "celib/fpumath.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_scenegprah_a0);
CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_component_a0);
CETECH_DECL_API(ct_material_a0);
CETECH_DECL_API(ct_hash_a0);
CETECH_DECL_API(ct_scene_a0);
CETECH_DECL_API(ct_yng_a0);
CETECH_DECL_API(ct_ydb_a0);

using namespace celib;

#define LOG_WHERE "mesh_renderer"

static uint64_t hash_combine(uint32_t a,
                             uint32_t b) {
    union {
        struct {
            uint32_t a;
            uint32_t b;
        };
        uint64_t ab;
    } c{
            .a = a,
            .b = b,
    };

    return c.ab;
}

struct mesh_data {
    uint64_t scene;
    uint64_t mesh[32];
    uint64_t node[32];
    uint64_t material[32];
    ct_material material_inst[32];
    uint32_t count;
};

namespace {
    struct WorldInstance {
        ct_world world;
        uint32_t n;
        uint32_t allocated;
        void *buffer;

        ct_entity *entity;
        mesh_data *mesh;
    };

    static struct MeshRendererGlobal {
        Map<uint32_t> world_map;
        Array<WorldInstance> world_instances;
        Map<uint32_t> ent_map;
        uint64_t type;
    } _G;

    void allocate(WorldInstance &_data,
                  cel_alloc *_allocator,
                  uint32_t sz) {
        //assert(sz > _data.n);

        WorldInstance new_data;
        const unsigned bytes = sz * (
                sizeof(ct_entity)
                + sizeof(mesh_data)
        );

        new_data.buffer = CEL_ALLOCATE(_allocator, char, bytes);
        new_data.n = _data.n;
        new_data.allocated = sz;

        new_data.entity = (ct_entity *) (new_data.buffer);
        new_data.mesh = (mesh_data *) (new_data.entity + sz);

        memcpy(new_data.entity, _data.entity, _data.n * sizeof(ct_entity));
        memcpy(new_data.mesh, _data.mesh, _data.n * sizeof(mesh_data));

        CEL_FREE(_allocator, _data.buffer);

        _data = new_data;
    }

    WorldInstance *_get_world_instance(ct_world world) {
        uint32_t idx = map::get(_G.world_map, world.h, UINT32_MAX);

        if (idx != UINT32_MAX) {
            return &_G.world_instances[idx];
        }

        return nullptr;
    }


    void destroy(ct_world world,
                 ct_entity ent) {
        WorldInstance &_data = *_get_world_instance(world);

        uint64_t id = hash_combine(world.h, ent.h);
        uint32_t i = map::get(_G.ent_map, id, UINT32_MAX);

        if (i == UINT32_MAX) {
            return;
        }

        unsigned last = _data.n - 1;
        ct_entity e = _data.entity[i];
        ct_entity last_e = _data.entity[last];

        _data.entity[i] = _data.entity[last];
         _data.mesh[i] = _data.mesh[last];

        map::set(_G.ent_map, hash_combine(world.h, last_e.h), i);
        map::remove(_G.ent_map, hash_combine(world.h, e.h));

        --_data.n;
    }

    static void _new_world(ct_world world) {
        uint32_t idx = array::size(_G.world_instances);
        array::push_back(_G.world_instances, WorldInstance());
        _G.world_instances[idx].world = world;
        map::set(_G.world_map, world.h, idx);
    }


    static void _destroy_world(ct_world world) {
        uint32_t idx = map::get(_G.world_map, world.h, UINT32_MAX);
        uint32_t last_idx = array::size(_G.world_instances) - 1;

        ct_world last_world = _G.world_instances[last_idx].world;

        CEL_FREE(ct_memory_a0.main_allocator(),
                 _G.world_instances[idx].buffer);

        _G.world_instances[idx] = _G.world_instances[last_idx];
        map::set(_G.world_map, last_world.h, idx);
        array::pop_back(_G.world_instances);
    }


    int _mesh_component_compiler(const char *filename,
                                 uint64_t *component_key,
                                 uint32_t component_key_count,
                                 struct ct_blob *data) {

        struct mesh_data t_data;

        uint64_t keys[component_key_count + 3];
        memcpy(keys, component_key, sizeof(uint64_t) * component_key_count);

        keys[component_key_count] = ct_yng_a0.calc_key("scene");
        t_data.scene = CT_ID64_0(ct_ydb_a0.get_string(
                filename, keys, component_key_count + 1, ""));

        uint64_t geom[32] = {};
        uint32_t geom_keys_count = 0;

        keys[component_key_count] = ct_yng_a0.calc_key("geometries");
        ct_ydb_a0.get_map_keys(filename,
                               keys, component_key_count + 1,
                               geom, CETECH_ARRAY_LEN(geom),
                               &geom_keys_count);

        for (uint32_t i = 0; i < geom_keys_count; ++i) {
            keys[component_key_count+1] = geom[i];

            keys[component_key_count+2] = ct_yng_a0.calc_key("mesh");
            t_data.mesh[i] = CT_ID64_0(
                    ct_ydb_a0.get_string(filename, keys,
                                         component_key_count+3, ""));

            keys[component_key_count+2] = ct_yng_a0.calc_key("material");
            t_data.material[i] = CT_ID64_0(
                    ct_ydb_a0.get_string(filename, keys,
                                         component_key_count+3, ""));

            keys[component_key_count+2] = ct_yng_a0.calc_key("node");
            t_data.node[i] = CT_ID64_0(
                    ct_ydb_a0.get_string(filename, keys,
                                         component_key_count+3, ""));
        }

        t_data.count = geom_keys_count;

        data->push(data->inst, (uint8_t *) &t_data, sizeof(struct mesh_data));

        return 1;
    }

    static void _on_world_create(ct_world world) {
        _new_world(world);
    }

    static void _on_world_destroy(ct_world world) {
        _destroy_world(world);
    }

    static void _destroyer(ct_world world,
                           ct_entity *ents,
                           uint32_t ent_count) {
        for (uint32_t i = 0; i < ent_count; i++) {
            destroy(world, ents[i]);
        }
    }


    ct_mesh_renderer mesh_create(ct_world world,
                                 ct_entity entity,
                                 uint64_t scene,
                                 uint64_t *mesh,
                                 uint64_t *node,
                                 uint64_t *material,
                                 uint32_t geom_count) {

        WorldInstance *data = _get_world_instance(world);

        uint32_t idx = data->n;
        allocate(*data, ct_memory_a0.main_allocator(), data->n + 1);
        ++data->n;

        map::set(_G.ent_map, hash_combine(world.h, entity.h), idx);

        ct_scene_a0.create_graph(world, entity, scene);

        data->entity[idx] = entity;
        mesh_data *mesh_data = &data->mesh[idx];
        *mesh_data = {};

        mesh_data->scene = scene;
        mesh_data->count = geom_count;
        for (uint32_t i = 0; i < geom_count; ++i) {
            uint64_t n = node[i];
            if (n == 0) {
                n = ct_scene_a0.get_mesh_node(scene, mesh[i]);
            }

            mesh_data->mesh[i] = mesh[i];
            mesh_data->node[i] = n;
            mesh_data->material[i] = material[i];
            mesh_data->material_inst[i] = ct_material_a0.resource_create(
                    material[i]);
        }

        return (ct_mesh_renderer) {.idx = idx, .world = world};
    }

    static void _spawner(ct_world world,
                         ct_entity *ents,
                         uint32_t *cents,
                         uint32_t *ents_parent,
                         uint32_t ent_count,
                         void *data) {
        CEL_UNUSED(ents_parent);

        struct mesh_data *tdata = (mesh_data *) data;

        for (uint32_t i = 0; i < ent_count; ++i) {
            mesh_create(world,
                        ents[cents[i]],
                        tdata[i].scene,
                        tdata[i].mesh,
                        tdata[i].node,
                        tdata[i].material,
                        tdata[i].count
            );
        }
    }
}


int mesh_is_valid(ct_mesh_renderer mesh) {
    return mesh.idx != UINT32_MAX;
}

int mesh_has(ct_world world,
             ct_entity entity) {
    uint64_t idx = hash_combine(world.h, entity.h);

    return map::has(_G.ent_map, idx);
}

ct_mesh_renderer mesh_get(ct_world world,
                          ct_entity entity) {

    uint64_t idx = hash_combine(world.h, entity.h);

    uint32_t component_idx = map::get(_G.ent_map, idx, UINT32_MAX);

    return {.idx = component_idx, .world = world};
}

void mesh_render_all(ct_world world,
                     uint8_t viewid,
                     uint64_t layer_name) {
    WorldInstance *data = _get_world_instance(world);

    for (uint32_t i = 0; i < data->n; ++i) {
        mesh_data *mesh_data = &data->mesh[i];

        uint64_t scene = mesh_data->scene;

        ct_entity ent = data->entity[i];

        ct_transform t = ct_transform_a0.get(world, ent);

        float wm[16];

        ct_transform_a0.get_world_matrix(t, wm);

        for (int j = 0; j < mesh_data->count; ++j) {
            ct_material material = mesh_data->material_inst[j];
            uint64_t geom = mesh_data->mesh[j];

            //mat44f_t t_w = MAT44F_INIT_IDENTITY;//*transform_get_world_matrix(world, t);
            float node_w[16];
            float final_w[16];

            celib::mat4_identity(node_w);
            celib::mat4_identity(final_w);

            if (ct_scenegprah_a0.has(world, ent)) {
                uint64_t name = ct_scene_a0.get_mesh_node(scene, geom);
                if (name != 0) {
                    ct_scene_node n = ct_scenegprah_a0.node_by_name(world, ent,
                                                                    name);
                    ct_scenegprah_a0.get_world_matrix(n, node_w);
                }
            }

            celib::mat4_mul(final_w, node_w, wm);

            bgfx::setTransform(&final_w, 1);

            ct_scene_a0.setVBIB(scene, geom);

            ct_material_a0.submit(material, layer_name, viewid);
        }
    }
}

ct_material mesh_get_material(ct_mesh_renderer mesh,
                              uint32_t idx) {
    WorldInstance *data = _get_world_instance(mesh.world);
    return data->mesh[mesh.idx].material_inst[idx];

}

void mesh_set_material(ct_mesh_renderer mesh,
                       uint32_t idx,
                       uint64_t material) {
    WorldInstance *data = _get_world_instance(mesh.world);
    ct_material material_instance = ct_material_a0.resource_create(material);

    data->mesh[mesh.idx].material_inst[idx] = material_instance;
}


static void _set_property(ct_world world,
                          ct_entity entity,
                          uint64_t key,
                          ct_property_value value) {
//
//    uint64_t scene = CT_ID64_0("scene");
//    uint64_t mesh = CT_ID64_0("mesh");
//    uint64_t node = CT_ID64_0("node");
    uint64_t material = CT_ID64_0("material");

//    ct_mesh_renderer mesh_renderer = mesh_get(world, entity);
    if (key == material) {
        //TODO: mesh_set_material(mesh_renderer, CT_ID64_0(value.value.str));
    }
}

static ct_property_value _get_property(ct_world world,
                                       ct_entity entity,
                                       uint64_t key) {
    CEL_UNUSED(world, entity, key);
//    uint64_t scene = CT_ID64_0("scene");
//    uint64_t mesh = CT_ID64_0("mesh");
//    uint64_t node = CT_ID64_0("node");
//    uint64_t material = CT_ID64_0("material");
//
//    ct_mesh_renderer mesh_r = mesh_get(world, entity);
//    WorldInstance *data = _get_world_instance(world);
//
//    char name_buff[256] = {};

//    if (key.id == scene.id) {
//        ResourceApiV0.get_filename(name_buff, CEL_ARRAY_LEN(name_buff), scene, ARRAY_AT(&data->scene, mesh_r.idx));
//        char* name = cel_strdup(name_buff, MemSysApiV0.main_scratch_allocator());
//
//        return (ct_property_value) {
//                .type= PROPERTY_STRING,
//                .value.str = name
//        };
//    } else if (key.id == mesh.id) {
//        ResourceApiV0.compiler_get_filename(name_buff, CEL_ARRAY_LEN(name_buff), scene, ARRAY_AT(&data->mesh, mesh_r.idx));
//        char* name = cel_strdup(name_buff, MemSysApiV0.main_scratch_allocator());
//
//        return (ct_property_value) {
//                .type= PROPERTY_STRING,
//                .value.str = name
//        };
//    } else if (key.id == node.id) {
//        ResourceApiV0.compiler_get_filename(name_buff, CEL_ARRAY_LEN(name_buff), scene, ARRAY_AT(&data->node, mesh_r.idx));
//        char* name = cel_strdup(name_buff, MemSysApiV0.main_scratch_allocator());
//
//        return (ct_property_value) {
//                .type= PROPERTY_STRING,
//                .value.str = name
//        };
//    } else if (key.id == material.id) {
//        ResourceApiV0.compiler_get_filename(name_buff, CEL_ARRAY_LEN(name_buff), scene, ARRAY_AT(&data->scene, mesh_r.idx));
//        char* name = cel_strdup(name_buff, MemSysApiV0.main_scratch_allocator());
//
//        return (ct_property_value) {
//                .type= PROPERTY_STRING,
//                .value.str = name
//        };
//    }

    return (ct_property_value) {.type= PROPERTY_INVALID};
}


static void _init_api(struct ct_api_a0 *api) {
    static struct ct_mesh_renderer_a0 _api = {
            .is_valid = mesh_is_valid,
            .has = mesh_has,
            .get = mesh_get,
            .create = mesh_create,
            .get_material = mesh_get_material,
            .set_material = mesh_set_material,
            .render_all = mesh_render_all,
    };


    api->register_api("ct_mesh_renderer_a0", &_api);
}


static void _init(ct_api_a0 *api) {
    _init_api(api);

    _G = {};

    _G.world_map.init(ct_memory_a0.main_allocator());
    _G.world_instances.init(ct_memory_a0.main_allocator());
    _G.ent_map.init(ct_memory_a0.main_allocator());
    _G.type = CT_ID64_0("mesh_renderer");

    ct_component_a0.register_compiler(_G.type, _mesh_component_compiler, 10);

    ct_component_a0.register_type(_G.type, {
            .spawner=_spawner,
            .destroyer=_destroyer,

            .world_clb.on_created = _on_world_create,
            .world_clb.on_destroy = _on_world_destroy,

            .set_property=_set_property, .get_property=_get_property
    });
}

static void _shutdown() {
    _G.world_map.destroy();
    _G.world_instances.destroy();
    _G.ent_map.destroy();
}

namespace mesh {
    void init(ct_api_a0 *api) {
        _init(api);
    }

    void shutdown() {
        _shutdown();
    }
}

CETECH_MODULE_DEF(
        mesh_renderer,
        {
            CETECH_GET_API(api, ct_component_a0);
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_scenegprah_a0);
            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_hash_a0);
            CETECH_GET_API(api, ct_material_a0);
            CETECH_GET_API(api, ct_yng_a0);
            CETECH_GET_API(api, ct_ydb_a0);
            CETECH_GET_API(api, ct_scene_a0);

        },
        {
            CEL_UNUSED(reload);
            mesh::init(api);
        },
        {
            CEL_UNUSED(reload);
            CEL_UNUSED(api);

            mesh::shutdown();
        }
)