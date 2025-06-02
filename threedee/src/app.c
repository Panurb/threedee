#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "app.h"

#include <camera.h>
// #include <SDL_mixer.h>
#include <resources.h>
#include <sound.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "settings.h"
#include "interface.h"
#include "linalg.h"
#include "render.h"
#include "scene.h"


App app;

static String version = "0.0";

static int debug_level = 0;


void create_game_window() {
    LOG_INFO("Creating game window");

    app.window = SDL_CreateWindow("ThreeDee", game_settings.width, game_settings.height, 0);
    SDL_SetWindowFullscreen(app.window, game_settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    SDL_SetWindowRelativeMouseMode(app.window, true);

    LOG_INFO("Game window created");
}


void destroy_game_window() {
    SDL_DestroyWindow(app.window);
    app.window = NULL;

    // SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipeline);
    // pipeline = NULL;
    // SDL_ReleaseGPUBuffer(app.gpu_device, vertex_buffer);
    // vertex_buffer = NULL;
}


void resize_game_window() {
    int w;
    int h;
    SDL_GetWindowSize(app.window, &w, &h);
    if (game_settings.width != w || game_settings.height != h) {
        SDL_SetWindowSize(app.window, game_settings.width, game_settings.height);
    }

    SDL_SetWindowFullscreen(app.window, game_settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}


void init() {
    setbuf(stdout, NULL);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO);
    // SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    TTF_Init();
    Mix_OpenAudio(0, NULL);
    Mix_AllocateChannels(32);

    memset(app.controllers, 0, sizeof(app.controllers));

    int controllers = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&controllers);
    free(joysticks);

    for (int i = 0; i < controllers; i++) {
        if (SDL_IsGamepad(i)) {
            if (app.controllers[i] == NULL) {
                app.controllers[i] = SDL_OpenGamepad(i);
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        app.player_controllers[i] = CONTROLLER_NONE;
    }
    app.player_controllers[0] = CONTROLLER_MKB;

    app.fps = FpsCounter_create();

    app.quit = false;
    app.focus = true;
    app.time_step = 1.0f / 60.0f;
    app.delta = 0.0f;
    app.state = STATE_GAME;
    app.base_path = SDL_GetBasePath();

    create_game_window();
    init_render();
    load_resources();
    create_scene();
}


void quit() {
    free(app.fps);
    destroy_game_window();

    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
}


void input_game(SDL_Event sdl_event) {
    if (sdl_event.type == SDL_EVENT_KEY_DOWN && sdl_event.key.key == SDLK_ESCAPE) {
        app.quit = true;
    }

    switch (app.state) {
        case STATE_GAME:
            if (sdl_event.type == SDL_EVENT_KEY_DOWN && sdl_event.key.repeat == 0) {
                if (sdl_event.key.key == SDLK_F1) {
                    if (game_settings.debug) {
                        debug_level = (debug_level + 1) % 4;
                    }
                }
            }
        default:
            break;
    }


    TransformComponent* trans = TransformComponent_get(scene->camera);
    float sens = game_settings.mouse_sensitivity / 1000.0f;

    if (sdl_event.type == SDL_EVENT_MOUSE_MOTION) {
        trans->rotation.roll = clamp(trans->rotation.roll - sdl_event.motion.yrel * sens, -M_PI_2, M_PI_2);
        trans->rotation.pitch = trans->rotation.pitch - sdl_event.motion.xrel * sens;
    }
}


void input() {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event))
    {
        switch (sdl_event.type) {
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                app.focus = false;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                app.focus = true;
                break;
            case SDL_EVENT_QUIT:
                app.quit = true;
                return;
            case SDL_EVENT_JOYSTICK_ADDED:
                if (app.controllers[sdl_event.jdevice.which] == NULL) {
                    app.controllers[sdl_event.jdevice.which] = SDL_OpenGamepad(sdl_event.jdevice.which);
                    LOG_INFO("Joystick added: %d", sdl_event.jdevice.which)
                }
                break;
            case SDL_EVENT_JOYSTICK_REMOVED:
                for (int i = 0; i < 8; i++) {
                    if (app.controllers[i] == NULL) continue;

                    if (SDL_GetGamepadID(app.controllers[i]) == sdl_event.jdevice.which) {
                        SDL_CloseGamepad(app.controllers[i]);
                        app.controllers[i] = NULL;
                        LOG_INFO("Joystick removed: %d", sdl_event.jdevice.which)
                    }
                }
                break;
            default:
                input_game(sdl_event);
                break;
        }
    }


    TransformComponent* trans = TransformComponent_get(scene->camera);
    Vector2 velocity = zeros2();

    const bool* keyboard_state = SDL_GetKeyboardState(NULL);

    if (keyboard_state[SDL_SCANCODE_W]) {
        velocity.y = -1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_S]) {
        velocity.y = 1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_A]) {
        velocity.x = -1.0f;
    }
    if (keyboard_state[SDL_SCANCODE_D]) {
        velocity.x = 1.0f;
    }

    velocity = mult(0.01f, normalized2(velocity));
    velocity = rotate(velocity, -trans->rotation.pitch);
    trans->position = sum3(trans->position, ( Vector3 ) { velocity.x, 0.0f, velocity.y });
}


void update(float time_step) {
    static AppState previous_state = STATE_MENU;

    AppState state = app.state;

    switch (app.state) {
        case STATE_GAME:
            break;
    }

    if (state != app.state) {
        previous_state = state;
    }
}


void draw() {
    LOG_DEBUG("Start draw");

    static float angle = 0.0f;
    angle += 0.01f;

    render();

    LOG_DEBUG("End draw");
}


void play_audio() {
    static float music_fade = 0.0f;
    static int current_music = -1;

    switch(app.state) {
        case STATE_GAME:
            break;
    }

    // if (game_data->music != current_music) {
    //     music_fade = fmaxf(0.0f, music_fade - 0.01f);
    //
    //     if (music_fade == 0.0f) {
    //         current_music = game_data->music;
    //
    //         Mix_HaltMusic();
    //         if (game_data->music != -1) {
    //             Mix_PlayMusic(resources.music[game_data->music], -1);
    //         }
    //     }
    // } else {
    //     music_fade = fminf(1.0f, music_fade + 0.01f);
    // }
    //
    // play_sounds(game_data->camera);

    Mix_VolumeMusic(0.5f * game_settings.music * music_fade);
}
