//==============================================================================
// Includes
//==============================================================================



#include "cetech/kernel/memory/memory.h"
#include "cetech/kernel/module/module.h"
#include "cetech/kernel/api/api_system.h"
#include "cetech/kernel/log/log.h"

#include "cetech/engine/machine/machine.h"

#include <include/SDL2/SDL.h>
#include <cetech/kernel/hashlib/hashlib.h>
#include <cetech/kernel/ebus/ebus.h>
#include <cetech/engine/renderer/renderer.h>
#include <cetech/engine/controlers/mouse.h>
#include <cetech/engine/controlers/keyboard.h>
#include <cetech/engine/controlers/gamepad.h>
#include <cetech/kernel/os/window.h>
#include <cetech/kernel/kernel.h>
#include <cetech/kernel/macros.h>


CETECH_DECL_API(ct_log_a0);
CETECH_DECL_API(ct_api_a0);
CETECH_DECL_API(ct_hashlib_a0);
CETECH_DECL_API(ct_ebus_a0);
CETECH_DECL_API(ct_cdb_a0);


//==============================================================================
// Extern functions
//==============================================================================

//==============================================================================
// Keyboard part
//==============================================================================


//==============================================================================
// Gamepad part
//==============================================================================

extern int sdl_window_init(struct ct_api_a0 *api);

extern void sdl_window_shutdown();

//==============================================================================
// Defines
//==============================================================================

#define LOG_WHERE "machine"

//==============================================================================
// Globals
//==============================================================================

#define is_button_down(now, last) ((now) && !(last))
#define is_button_up(now, last)   (!(now) && (last))


static struct MachineGlobals {
    struct {
        uint8_t state[MOUSE_BTN_MAX];
        float position[2];
    } mouse;

    struct {
        SDL_GameController *controller[GAMEPAD_MAX];
        SDL_Haptic *haptic[GAMEPAD_MAX];

        float position[GAMEPAD_MAX][GAMEPAD_AXIX_MAX][2];
        int state[GAMEPAD_MAX][GAMEPAD_BTN_MAX];
    } controlers;

    struct {
        uint8_t state[KEY_MAX];
    } keyboard;
} _G = {};

CETECH_DECL_API(ct_memory_a0);

//==============================================================================
// Interface
//==============================================================================


void sdl_mouse_process() {
    int pos[2] = {};

    uint32_t state = SDL_GetMouseState(&pos[0], &pos[1]);

    uint8_t curent_state[MOUSE_BTN_MAX] = {};

    curent_state[MOUSE_BTN_LEFT] = (uint8_t) (state & SDL_BUTTON_LMASK);
    curent_state[MOUSE_BTN_RIGHT] = (uint8_t) (state & SDL_BUTTON_RMASK);
    curent_state[MOUSE_BTN_MIDLE] = (uint8_t) (state & SDL_BUTTON_MMASK);

    struct ct_renderer_a0 *renderer_a0 = (struct ct_renderer_a0 *) ct_api_a0.first(
            "ct_renderer_a0").api;

    uint32_t window_size[2] = {};
    renderer_a0->get_size(&window_size[0], &window_size[1]);

    _G.mouse.position[0] = pos[0];
    _G.mouse.position[1] = window_size[1] - pos[1];


    struct ct_cdb_obj_t *event;
    event = ct_cdb_a0.create_object(ct_cdb_a0.global_db(),
                                    EVENT_MOUSE_MOVE);

    struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
    ct_cdb_a0.set_vec3(w, CT_ID64_0("position"), _G.mouse.position);
    ct_cdb_a0.write_commit(w);


    ct_ebus_a0.broadcast(MOUSE_EBUS, event);

    for (uint32_t i = 0; i < MOUSE_BTN_MAX; ++i) {
        if (is_button_down(curent_state[i], _G.mouse.state[i])) {
            event = ct_cdb_a0.create_object(ct_cdb_a0.global_db(),
                                            EVENT_MOUSE_DOWN);

            w = ct_cdb_a0.write_begin(event);
            ct_cdb_a0.set_uint32(w, CT_ID64_0("button"), i);
            ct_cdb_a0.write_commit(w);

            ct_ebus_a0.broadcast(MOUSE_EBUS, event);

        } else if (is_button_up(curent_state[i], _G.mouse.state[i])) {
            event = ct_cdb_a0.create_object(ct_cdb_a0.global_db(),
                                            EVENT_MOUSE_UP);

            w = ct_cdb_a0.write_begin(event);
            ct_cdb_a0.set_uint32(w, CT_ID64_0("button"), i);
            ct_cdb_a0.write_commit(w);

            ct_ebus_a0.broadcast(MOUSE_EBUS, event);
        }

        _G.mouse.state[i] = curent_state[i];
    }
}

void sdl_keyboard_process() {
    const uint8_t *state = SDL_GetKeyboardState(NULL);


    struct ct_cdb_obj_t *event;
    for (uint32_t i = 0; i < KEY_MAX; ++i) {
        if (is_button_down(state[i], _G.keyboard.state[i])) {
            event = ct_cdb_a0.create_object(ct_cdb_a0.global_db(),
                                            EVENT_KEYBOARD_DOWN);

            struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
            ct_cdb_a0.set_uint32(w, CT_ID64_0("keycode"), i);
            ct_cdb_a0.write_commit(w);

            ct_ebus_a0.broadcast(KEYBOARD_EBUS, event);


        } else if (is_button_up(state[i], _G.keyboard.state[i])) {
            event = ct_cdb_a0.create_object(ct_cdb_a0.global_db(),
                                            EVENT_KEYBOARD_UP);

            struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
            ct_cdb_a0.set_uint32(w, CT_ID64_0("keycode"), i);
            ct_cdb_a0.write_commit(w);

            ct_ebus_a0.broadcast(KEYBOARD_EBUS, event);

        }

        _G.keyboard.state[i] = state[i];
    }
}

int _new_controler() {
    for (int i = 0; i < GAMEPAD_MAX; ++i) {
        if (_G.controlers.controller[i] != NULL) {
            continue;
        }

        return i;
    }

    return -1;
}

void _remove_controler(int idx) {
    ct_log_a0.info("controlers.gamepad", "Remove gamepad %d.", idx);

    SDL_HapticClose(_G.controlers.haptic[idx]);
    SDL_GameControllerClose(_G.controlers.controller[idx]);

    _G.controlers.controller[idx] = NULL;
    _G.controlers.haptic[idx] = NULL;
}

int _create_controler(int i) {
    SDL_GameController *controller = SDL_GameControllerOpen(i);
    SDL_Joystick *joy = SDL_GameControllerGetJoystick(controller);

    int idx = _new_controler();

    _G.controlers.controller[idx] = controller;

    memset(_G.controlers.state[idx], 0, sizeof(int) * GAMEPAD_BTN_MAX);

    ct_log_a0.info("controlers.gamepad", "Add gamepad %d.", i);

    if (SDL_JoystickIsHaptic(joy) == 1) {
        SDL_Haptic *haptic = SDL_HapticOpenFromJoystick(joy);
        SDL_HapticRumbleInit(haptic);
        _G.controlers.haptic[idx] = haptic;

        ct_log_a0.info("controlers.gamepad", "Gamepad %d has haptic support.",
                       i);
    } else {
        _G.controlers.haptic[idx] = NULL;
    }

    return idx;
}

static SDL_GameControllerButton _btn_to_sdl[GAMEPAD_BTN_MAX] = {
        [GAMEPAD_BTN_INVALID] = SDL_CONTROLLER_BUTTON_INVALID,
        [GAMEPAD_BTN_A] =               SDL_CONTROLLER_BUTTON_A,
        [GAMEPAD_BTN_B] =               SDL_CONTROLLER_BUTTON_B,
        [GAMEPAD_BTN_X] =               SDL_CONTROLLER_BUTTON_X,
        [GAMEPAD_BTN_Y] =               SDL_CONTROLLER_BUTTON_Y,
        [GAMEPAD_BTN_BACK] =            SDL_CONTROLLER_BUTTON_BACK,
        [GAMEPAD_BTN_GUIDE] =           SDL_CONTROLLER_BUTTON_GUIDE,
        [GAMEPAD_BTN_START] =           SDL_CONTROLLER_BUTTON_START,
        [GAMEPAD_BTN_LEFTSTICK] =       SDL_CONTROLLER_BUTTON_LEFTSTICK,
        [GAMEPAD_BTN_RIGHTSTICK] =      SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        [GAMEPAD_BTN_LEFTSHOULDER] =    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        [GAMEPAD_BTN_RIGHTSHOULDER] =   SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        [GAMEPAD_BTN_DPADUP] =          SDL_CONTROLLER_BUTTON_DPAD_UP,
        [GAMEPAD_BTN_DPADDOWN] =        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        [GAMEPAD_BTN_DPADLEFT] =        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        [GAMEPAD_BTN_DPADRIGHT] =       SDL_CONTROLLER_BUTTON_DPAD_RIGHT
};

static SDL_GameControllerAxis _axis_to_sdl[GAMEPAD_AXIX_MAX][2] = {
        [GAMEPAD_AXIS_INVALID] = {SDL_CONTROLLER_AXIS_INVALID,
                                  SDL_CONTROLLER_AXIS_INVALID},
        [GAMEPAD_AXIS_LEFT] = {SDL_CONTROLLER_AXIS_LEFTX,
                               SDL_CONTROLLER_AXIS_LEFTY},
        [GAMEPAD_AXIS_RIGHT] = {SDL_CONTROLLER_AXIS_RIGHTX,
                                SDL_CONTROLLER_AXIS_RIGHTY},
        [GAMEPAD_AXIS_TRIGER] = {SDL_CONTROLLER_AXIS_TRIGGERLEFT,
                                 SDL_CONTROLLER_AXIS_TRIGGERRIGHT},
};

void sdl_gamepad_process() {
    int curent_state[GAMEPAD_MAX][GAMEPAD_AXIX_MAX] = {};
    float curent_pos[GAMEPAD_MAX][GAMEPAD_BTN_MAX][2] = {};

    for (int i = 0; i < GAMEPAD_MAX; ++i) {
        if (_G.controlers.controller[i] == NULL) {
            continue;
        }

        for (int j = 0; j < GAMEPAD_BTN_MAX; ++j) {
            curent_state[i][j] = SDL_GameControllerGetButton(
                    _G.controlers.controller[i],
                    _btn_to_sdl[j]);
        }

        for (int j = 0; j < GAMEPAD_AXIX_MAX; ++j) {
            int16_t x = SDL_GameControllerGetAxis(_G.controlers.controller[i],
                                                  _axis_to_sdl[j][0]);
            int16_t y = SDL_GameControllerGetAxis(_G.controlers.controller[i],
                                                  _axis_to_sdl[j][1]);

            float coef = 1.0f / INT16_MAX;
            float x_norm = x * coef;
            float y_norm = y * coef;

            curent_pos[i][j][0] = x_norm;
            curent_pos[i][j][1] = -y_norm;
        }
    }

    for (int i = 0; i < GAMEPAD_MAX; ++i) {
        if (_G.controlers.controller[i] == NULL) {
            continue;
        }

        for (int j = 0; j < GAMEPAD_BTN_MAX; ++j) {
            struct ct_gamepad_btn_event event;
            event.gamepad_id = i;
            event.button = j;

            if (is_button_down(curent_state[i][j], _G.controlers.state[i][j])) {
                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_GAMEPAD_DOWN);

                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("gamepad_id"), i);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("button"), j);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(GAMEPAD_EBUS, event);


            } else if (is_button_up(curent_state[i][j],
                                    _G.controlers.state[i][j])) {
                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_GAMEPAD_UP);

                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("gamepad_id"), i);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("button"), j);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(GAMEPAD_EBUS, event);

            }

            _G.controlers.state[i][j] = curent_state[i][j];
        }

        for (int j = 0; j < GAMEPAD_AXIX_MAX; ++j) {
            float pos[2] = {curent_pos[i][j][0], curent_pos[i][j][1]};


            if ((pos[0] != _G.controlers.position[i][j][0]) ||
                (pos[1] != _G.controlers.position[i][j][1])) {

                _G.controlers.position[i][j][0] = pos[0];
                _G.controlers.position[i][j][1] = pos[1];

                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_GAMEPAD_MOVE);

                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("gamepad_id"), i);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("axis"), j);
                ct_cdb_a0.set_vec3(w, CT_ID64_0("position"), pos);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(GAMEPAD_EBUS, event);

            }
        }
    }
}

int sdl_gamepad_is_active(int idx) {
    return _G.controlers.controller[idx] != NULL;
}

void sdl_gamepad_play_rumble(int gamepad,
                             float strength,
                             uint32_t length) {

    SDL_Haptic *h = _G.controlers.haptic[gamepad];

    SDL_HapticRumblePlay(h, strength, length);
}

static void _update(float dt) {
    CT_UNUSED(dt);

    SDL_Event e = {0};

    while (SDL_PollEvent(&e) > 0) {
        switch (e.type) {
            case SDL_QUIT: {
                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        KERNEL_QUIT_EVENT);

                ct_ebus_a0.broadcast(KERNEL_EBUS, event);
            }
                break;

            case SDL_WINDOWEVENT: {
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                                ct_cdb_a0.global_db(),
                                EVENT_WINDOW_RESIZED);

                        struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                        ct_cdb_a0.set_uint32(w, CT_ID64_0("window_id"),
                                             e.window.windowID);
                        ct_cdb_a0.set_uint32(w, CT_ID64_0("width"),
                                             e.window.data1);
                        ct_cdb_a0.set_uint32(w, CT_ID64_0("height"),
                                             e.window.data2);
                        ct_cdb_a0.write_commit(w);

                        ct_ebus_a0.broadcast(WINDOW_EBUS, event);

                    }
                        break;
                }
            }
                break;

            case SDL_MOUSEWHEEL: {
                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_MOUSE_WHEEL);

                float pos[3] = {e.wheel.x, e.wheel.y};

                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);;
                ct_cdb_a0.set_vec3(w, CT_ID64_0("position"), pos);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(MOUSE_EBUS, event);

            }
                break;


            case SDL_TEXTINPUT: {
                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_KEYBOARD_TEXT);


                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                ct_cdb_a0.set_string(w, CT_ID64_0("text"), e.text.text);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(KEYBOARD_EBUS, event);

            }
                break;

            case SDL_CONTROLLERDEVICEADDED: {
                int idx = _create_controler(e.cdevice.which);

                struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                        ct_cdb_a0.global_db(),
                        EVENT_GAMEPAD_CONNECT);

                struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                ct_cdb_a0.set_uint32(w, CT_ID64_0("gamepad_id"), idx);
                ct_cdb_a0.write_commit(w);

                ct_ebus_a0.broadcast(GAMEPAD_EBUS, event);

            }
                break;

            case SDL_CONTROLLERDEVICEREMOVED: {
                for (int i = 0; i < GAMEPAD_MAX; ++i) {
                    SDL_GameController *controller;
                    controller = SDL_GameControllerFromInstanceID(
                            e.cdevice.which);

                    if (_G.controlers.controller[i] != controller) {
                        continue;
                    }

                    _remove_controler(i);

                    struct ct_cdb_obj_t *event = ct_cdb_a0.create_object(
                            ct_cdb_a0.global_db(),
                            EVENT_GAMEPAD_DISCONNECT);

                    struct ct_cdb_obj_t *w = ct_cdb_a0.write_begin(event);
                    ct_cdb_a0.set_uint32(w, CT_ID64_0("gamepad_id"), i);
                    ct_cdb_a0.write_commit(w);

                    ct_ebus_a0.broadcast(GAMEPAD_EBUS, event);

                    break;
                }
            }
                break;


            default:
                break;
        }
    }

    sdl_gamepad_process();
    sdl_mouse_process();
    sdl_keyboard_process();

}

static struct ct_machine_a0 a0 = {
        .update = _update,
        .gamepad_is_active = sdl_gamepad_is_active,
        .gamepad_play_rumble = sdl_gamepad_play_rumble,
};

static void init(struct ct_api_a0 *api) {
    api->register_api("ct_machine_a0", &a0);

    CETECH_GET_API(api, ct_memory_a0);
    CETECH_GET_API(api, ct_log_a0);
    CETECH_GET_API(api, ct_hashlib_a0);
    CETECH_GET_API(api, ct_ebus_a0);
    CETECH_GET_API(api, ct_cdb_a0);

    if (SDL_Init(SDL_INIT_VIDEO |
                 SDL_INIT_GAMECONTROLLER |
                 SDL_INIT_HAPTIC |
                 SDL_INIT_JOYSTICK) != 0) {
        //if (SDL_Init(0) != 0) {
        ct_log_a0.error(LOG_WHERE, "Could not init sdl - %s",
                        SDL_GetError());
        return; // TODO: dksandasdnask FUCK init without return ptype?????
    }

    sdl_window_init(api);

}

static void shutdown() {
    sdl_window_shutdown();

    SDL_Quit();
}

CETECH_MODULE_DEF(
        machine,
        {
            CETECH_GET_API(api, ct_log_a0);

            ct_api_a0 = *api;
        },
        {
            CT_UNUSED(reload);
            init(api);
        },
        {
            CT_UNUSED(reload);
            CT_UNUSED(api);
            shutdown();

        }
)
