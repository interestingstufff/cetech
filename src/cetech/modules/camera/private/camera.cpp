#include <cetech/engine/entity/entity.h>
#include <cetech/modules/renderer/renderer.h>
#include <cetech/modules/transform/transform.h>
#include <cetech/core/blob/blob.h>
#include <cetech/modules/camera/camera.h>
#include "celib/map.inl"

#include "cetech/core/yaml/yaml.h"
#include "cetech/core/hashlib/hashlib.h"
#include "cetech/core/config/config.h"
#include "cetech/core/memory/memory.h"
#include "cetech/core/api/api_system.h"
#include "celib/fpumath.h"
#include "cetech/core/module/module.h"

CETECH_DECL_API(ct_memory_a0);
CETECH_DECL_API(ct_component_a0);
CETECH_DECL_API(ct_transform_a0);
CETECH_DECL_API(ct_hash_a0);

using namespace celib;

namespace {

    struct camera_data {
        float near;
        float far;
        float fov;
    };


    struct WorldInstance {
        ct_world world;
        uint32_t n;
        uint32_t allocated;
        void *buffer;

        ct_entity *entity;
        float *near;
        float *far;
        float *fov;
    };

#define _G CameraGlobal
    static struct CameraGlobal {
        uint64_t type;

        Map<uint32_t> world_map;
        Array<WorldInstance> world_instances;
        Map<uint32_t> ent_map;
    } CameraGlobal;


    static void allocate(WorldInstance &_data,
                         cel_alloc *_allocator,
                         uint32_t sz) {
        //assert(sz > _data.n);

        WorldInstance new_data;
        const unsigned bytes = sz * (sizeof(ct_entity) + (3 * sizeof(float)));
        new_data.buffer = CEL_ALLOCATE(_allocator, char, bytes);
        new_data.n = _data.n;
        new_data.allocated = sz;

        new_data.entity = (ct_entity *) (new_data.buffer);
        new_data.near = (float *) (new_data.entity + sz);
        new_data.far = (float *) (new_data.near + sz);
        new_data.fov = (float *) (new_data.far + sz);

        memcpy(new_data.entity, _data.entity, _data.n * sizeof(ct_entity));
        memcpy(new_data.near, _data.near, _data.n * sizeof(float));
        memcpy(new_data.far, _data.far, _data.n * sizeof(float));
        memcpy(new_data.fov, _data.fov, _data.n * sizeof(float));

        CEL_FREE(_allocator, _data.buffer);

        _data = new_data;
    }

//    void destroy(WorldInstance &_data,
//                 unsigned i) {
//
//        unsigned last = _data.n - 1;
////        ct_entity e = _data.entity[i];
////        ct_entity last_e = _data.entity[last];
//
//        _data.entity[i] = _data.entity[last];
//        _data.near[i] = _data.near[last];
//        _data.far[i] = _data.far[last];
//        _data.fov[i] = _data.fov[last];
//
////    map::set(_G._map, last_e.h, i);
////    map::remove(_G._map, e.h);
//
//        --_data.n;
//    }

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

    static WorldInstance *_get_world_instance(ct_world world) {
        uint32_t idx = map::get(_G.world_map, world.h, UINT32_MAX);

        if (idx != UINT32_MAX) {
            return &_G.world_instances[idx];
        }

        return nullptr;
    }

    int _camera_component_compiler(yaml_node_t body,
                                   ct_blob *data) {

        struct camera_data t_data;

        YAML_NODE_SCOPE(near, body, "near", t_data.near = yaml_as_float(near););
        YAML_NODE_SCOPE(far, body, "far", t_data.far = yaml_as_float(far););
        YAML_NODE_SCOPE(fov, body, "fov", t_data.fov = yaml_as_float(fov););

        data->push(data->inst, (uint8_t *) &t_data, sizeof(t_data));

        return 1;
    }
}

namespace camera {
    int is_valid(ct_camera camera) {
        return camera.idx != UINT32_MAX;
    }

    void get_project_view(ct_camera camera,
                          float *proj,
                          float *view,
                          int width,
                          int height) {

        WorldInstance *world_inst = _get_world_instance(camera.world);

        ct_entity e = world_inst->entity[camera.idx];
        ct_transform t = ct_transform_a0.get(camera.world, e);

        float fov = world_inst->fov[camera.idx];
        float near = world_inst->near[camera.idx];
        float far = world_inst->far[camera.idx];

        celib::mat4_proj(proj, fov, float(width) / float(height), near, far,
                         true);

        float w[16];
        ct_transform_a0.get_world_matrix(t, w);

        //celib::mat4_move(view, w);
        celib::mat4_inverse(view, w);
    }

    int has(ct_world world,
            ct_entity entity) {

        uint32_t idx = world.h ^entity.h;

        return map::has(_G.ent_map, idx);
    }

    ct_camera get(ct_world world,
                  ct_entity entity) {

        uint32_t idx = world.h ^entity.h;
        uint32_t component_idx = map::get(_G.ent_map, idx, UINT32_MAX);

        return (ct_camera) {.idx = component_idx, .world = world};
    }

    ct_camera create(ct_world world,
                     ct_entity entity,
                     float near,
                     float far,
                     float fov) {

        WorldInstance *data = _get_world_instance(world);

        uint32_t idx = data->n;
        allocate(*data, ct_memory_a0.main_allocator(), data->n + 1);
        ++data->n;

        data->entity[idx] = entity;
        data->near[idx] = near;
        data->far[idx] = far;
        data->fov[idx] = fov;

        map::set(_G.ent_map, world.h ^ entity.h, idx);

        return (ct_camera) {.idx = idx, .world = world};
    }

}

namespace camera_module {

    static ct_camera_a0 camera_api = {
            .is_valid = camera::is_valid,
            .get_project_view = camera::get_project_view,
            .has = camera::has,
            .get = camera::get,
            .create = camera::create
    };

    void _on_world_create(ct_world world) {
        _new_world(world);
    }

    void _on_world_destroy(ct_world world) {
        _destroy_world(world);
    }

    void _destroyer(ct_world world,
                    ct_entity *ents,
                    uint32_t ent_count) {
        CEL_UNUSED(world);

        // TODO: remove from arrays, swap idx -> last AND change size
        for (uint32_t i = 0; i < ent_count; i++) {
            map::remove(_G.world_map, ents[i].h);
        }
    }

    void _spawner(ct_world world,
                  ct_entity *ents,
                  uint32_t *cents,
                  uint32_t *ents_parent,
                  uint32_t ent_count,
                  void *data) {

        CEL_UNUSED(ents_parent);

        camera_data *tdata = (camera_data *) data;

        for (uint32_t i = 0; i < ent_count; ++i) {
            camera::create(world,
                           ents[cents[i]],
                           tdata[i].near,
                           tdata[i].far,
                           tdata[i].fov);
        }
    }

    static void _init(ct_api_a0 *api) {
        api->register_api("ct_camera_a0", &camera_api);



        _G = {};

        _G.world_map.init(ct_memory_a0.main_allocator());
        _G.world_instances.init(ct_memory_a0.main_allocator());
        _G.ent_map.init(ct_memory_a0.main_allocator());

        _G.type = ct_hash_a0.id64_from_str("camera");

        ct_component_a0.register_compiler(_G.type,
                                          _camera_component_compiler,
                                          10);

        ct_component_a0.register_type(_G.type, {
                .spawner=_spawner,
                .destroyer=_destroyer,

                .world_clb.on_created = _on_world_create,
                .world_clb.on_destroy = _on_world_destroy
        });
    }

    static void _shutdown() {
        _G.ent_map.destroy();
        _G.world_instances.destroy();
        _G.world_map.destroy();
    }
}

CETECH_MODULE_DEF(
        camera,
        {
            CETECH_GET_API(api, ct_memory_a0);
            CETECH_GET_API(api, ct_component_a0);
            CETECH_GET_API(api, ct_transform_a0);
            CETECH_GET_API(api, ct_hash_a0);
        },
        {
            camera_module::_init(api);
        },
        {
            CEL_UNUSED(api);
            camera_module::_shutdown();
        }
)