#include <cstring>

#include "luajit/lua.hpp"

#include "common/asserts.h"
#include "lua/lua_enviroment.h"
#include "common/log/log.h"
#include "common/crypto/murmur_hash.inl.h"
#include "resource/resource_manager.h"
#include "device.h"

namespace cetech {
    class LuaEnviromentImlementation : public LuaEnviroment {
        friend class LuaEnviroment;

        lua_State* _state;

        LuaEnviromentImlementation() {
            _state = luaL_newstate();
            CE_CHECK_PTR(_state);

            luaL_openlibs(_state);

            lua_getfield(_state, LUA_GLOBALSINDEX, "package");
            lua_getfield(_state, -1, "loaders");
            lua_remove(_state, -2);

            int num_loaders = 0;
            lua_pushnil(_state);
            while (lua_next(_state, -2) != 0) {
                lua_pop(_state, 1);
                num_loaders++;
            }

            lua_pushinteger(_state, num_loaders + 1);
            lua_pushcfunction(_state, require);
            lua_rawset(_state, -3);
            lua_pop(_state, 1);
        }

        virtual ~LuaEnviromentImlementation() final {
            lua_close(_state);
        }

        virtual void execute_resource(const resource_lua::Resource* res) final {
            //lua_pushcfunction(_state, error_handler);

            if (luaL_dostring(_state, resource_lua::get_source(res))) {
                const char* last_error = lua_tostring(_state, -1);
                lua_pop(_state, 1);
                log::error("lua", "%s", last_error);
            }
        }

        virtual void execute_string(const char* str) final {
            //lua_pushcfunction(_state, error_handler);

            if (luaL_dostring(_state, str)) {
                const char* last_error = lua_tostring(_state, -1);
                lua_pop(_state, 1);
                log::error("lua", "%s", last_error);
            }
        }

        static int require(lua_State* L) {
            const char* name = lua_tostring( L, 1);
            StringId64_t name_hash = murmur_hash_64(name, strlen(name), 22);

            const resource_lua::Resource* res =
                (resource_lua::Resource*) device_globals::device().resource_manager().get(
                    resource_lua::type_hash(), name_hash);

            if (res == nullptr) {
                return 0;
            }

            luaL_loadstring(L, resource_lua::get_source(res));
            return 1;
        }

    };

    LuaEnviroment* LuaEnviroment::make(Allocator& alocator) {
        return MAKE_NEW(alocator, LuaEnviromentImlementation);
    }

    void LuaEnviroment::destroy(Allocator& alocator, LuaEnviroment* cs) {
        MAKE_DELETE(memory_globals::default_allocator(), LuaEnviroment, cs);
    }

    //     namespace internal {
    //         static int error_handler(lua_State* L) {
    //             lua_getfield( L, LUA_GLOBALSINDEX, "debug");
    //             if (!lua_istable( L, -1)) {
    //                 lua_pop( L, 1);
    //                 return 0;
    //             }
    //
    //             lua_getfield( L, -1, "traceback");
    //             if (!lua_isfunction( L, -1)) {
    //                 lua_pop( L, 2);
    //                 return 0;
    //             }
    //
    //             lua_pushvalue( L, 1);
    //             lua_pushinteger( L, 2);
    //             lua_call( L, 2, 1);
    //
    //             log::error("lua", lua_tostring( L, -1));
    //
    //             lua_pop( L, 1);
    //             lua_pop( L, 1);
    //
    //             return 0;
    //         }
    //
    //     }
}