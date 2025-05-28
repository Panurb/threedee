#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "app.h"

#include <camera.h>
#include <SDL_mixer.h>
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

    app.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

    LOG_INFO("Game window created");
}


void destroy_game_window() {
    SDL_DestroyWindow(app.window);
    app.window = NULL;

    // SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipeline);
    // pipeline = NULL;
    // SDL_ReleaseGPUBuffer(app.gpu_device, vertex_buffer);
    // vertex_buffer = NULL;

    SDL_DestroyGPUDevice(app.gpu_device);
    app.gpu_device = NULL;
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
    LOG_DEBUG("Start draw");

    static float angle = 0.0f;
    angle += 0.01f;

    SDL_GPUCommandBuffer* gpu_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    if (!gpu_command_buffer) {
        LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(gpu_command_buffer, app.window, &swapchain_texture, NULL, NULL);

    if (swapchain_texture) {
        // for (int i = 0; i < 20; i++) {
        //     Matrix4 transform = transform_matrix(zeros3(), (Rotation) { 0.0f, 0.0f, angle + i * (2.0f * M_PI / 20) }, ones3());
        //     add_render_instance(gpu_command_buffer, &render_modes.quad, transform);
        // }
        add_render_instance(gpu_command_buffer, &render_modes.cube, transform_matrix(zeros3(), (Rotation) { 0.0f, angle, 0.0f }, ones3()));

        SDL_GPUColorTargetInfo color_target_info = {
            .texture = swapchain_texture,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }
        };
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(gpu_command_buffer, &color_target_info, 1, NULL);

        // Matrix4 projection_matrix = CameraComponent_get(game_data->menu_camera)->projection_matrix;
        // SDL_PushGPUVertexUniformData(gpu_command_buffer, 1, &projection_matrix, sizeof(Matrix4));
        // render(gpu_command_buffer, render_pass, &render_modes.quad);

        Matrix4 projection_matrix = CameraComponent_get(game_data->camera)->projection_matrix;
        SDL_PushGPUVertexUniformData(gpu_command_buffer, 1, &projection_matrix, sizeof(Matrix4));
        render(gpu_command_buffer, render_pass, &render_modes.cube);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(gpu_command_buffer);

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
