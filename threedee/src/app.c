#include <stdio.h>

#include <SDL3/SDL.h>

#include "app.h"

#include <math.h>
#include <stdlib.h>

#include "game.h"
#include "settings.h"
#include "interface.h"
#include "menu.h"


App app;

static String version = "0.0";

static int debug_level = 0;


void create_game_window() {
    app.window = SDL_CreateWindow("ThreeDee", game_settings.width, game_settings.height, 0);

    SDL_GPUDevice* gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERSTAGE_FRAGMENT | SDL_GPU_SHADERSTAGE_VERTEX,  false, "vulkan");
    SDL_ClaimWindowForGPUDevice(gpu, app.window);

    SDL_SetWindowFullscreen(app.window, game_settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, game_settings.vsync ? "1" : "0");
    app.renderer = SDL_CreateRenderer(app.window, NULL);
}


void destroy_game_window() {
    SDL_DestroyWindow(app.window);
    app.window = NULL;

    // This will also destroy all textures
    SDL_DestroyRenderer(app.renderer);
    app.renderer = NULL;
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
    SDL_HideCursor();

    // IMG_Init(IMG_INIT_PNG);
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

    create_game_window();

    app.fps = FpsCounter_create();

    app.quit = false;
    app.focus = true;
    app.time_step = 1.0f / 60.0f;
    app.delta = 0.0f;
    app.state = STATE_GAME;
}


void quit() {
    free(app.fps);
    destroy_game_window();

    Mix_CloseAudio();
    TTF_Quit();
    // IMG_Quit();
    SDL_Quit();
}


void input_game(SDL_Event sdl_event) {
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
                break;
        }
    }
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
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);

    switch (app.state) {
        case STATE_GAME:
            break;
        default:
            break;
    }

    if (debug_level) {
        draw_debug(debug_level);
    }
    FPSCounter_draw(app.fps);

    SDL_RenderPresent(app.renderer);
}


void play_audio() {
    static float music_fade = 0.0f;
    static int current_music = -1;

    switch(app.state) {
        case STATE_MENU:
            game_data->music = 0;
            break;
        case STATE_GAME:
        case STATE_INTRO:
        case STATE_PAUSE:
            game_data->music = 1;
            break;
        default:
            game_data->music = -1;
            break;
    }

    if (game_data->music != current_music) {
        music_fade = fmaxf(0.0f, music_fade - 0.01f);

        if (music_fade == 0.0f) {
            current_music = game_data->music;

            Mix_HaltMusic();
            if (game_data->music != -1) {
                Mix_PlayMusic(resources.music[game_data->music], -1);
            }
        }
    } else {
        music_fade = fminf(1.0f, music_fade + 0.01f);
    }

    play_sounds(game_data->camera);

    Mix_VolumeMusic(0.5f * game_settings.music * music_fade);
}
