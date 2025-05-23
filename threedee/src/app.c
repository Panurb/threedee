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

static SDL_GPUGraphicsPipeline* pipeline;
static SDL_GPUBuffer* vertex_buffer;


typedef struct Vertex
{
    Vector3 position;
    Color color;
} Vertex;


SDL_GPUShader* load_shader(
	SDL_GPUDevice* device,
	const char* shader_filename,
	Uint32 sampler_count,
	Uint32 uniform_buffer_count,
	Uint32 storage_buffer_count,
	Uint32 storage_texture_count
) {
	// Auto-detect the shader stage from the file name for convenience
	SDL_GPUShaderStage stage;
	if (SDL_strstr(shader_filename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(shader_filename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Invalid shader stage!");
		return NULL;
	}

	char fullPath[256];
	SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entrypoint;

	if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sdata/shaders/compiled/SPIRV/%s.spv", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entrypoint = "main";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sdata/shaders/compiled/MSL/%s.msl", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entrypoint = "main0";
	} else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(fullPath, sizeof(fullPath), "%sdata/shaders/compiled/DXIL/%s.dxil", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entrypoint = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t codeSize;
	void* code = SDL_LoadFile(fullPath, &codeSize);
	if (code == NULL)
	{
		SDL_Log("Failed to load shader from disk! %s", fullPath);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shaderInfo = {
		.code = code,
		.code_size = codeSize,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = sampler_count,
		.num_uniform_buffers = uniform_buffer_count,
		.num_storage_buffers = storage_buffer_count,
		.num_storage_textures = storage_texture_count
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}


void create_game_window() {
    LOG_INFO("Creating game window");

    app.window = SDL_CreateWindow("ThreeDee", game_settings.width, game_settings.height, 0);
    SDL_SetWindowFullscreen(app.window, game_settings.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);

    app.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

    SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "PositionColor.vert", 0, 0, 0, 0);
    if (!vertex_shader) {
        LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
        return;
    }

    SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "SolidColor.frag", 0, 0, 0, 0);
    if (!fragment_shader) {
        LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
        return;
    }

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                .format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
            }},
        },
        // This is set up to match the vertex shader layout!
        .vertex_input_state = (SDL_GPUVertexInputState){
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
                .slot = 0,
                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                .instance_step_rate = 0,
                .pitch = sizeof(Vertex)
            }},
            .num_vertex_attributes = 2,
            .vertex_attributes = (SDL_GPUVertexAttribute[]){{
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
                .location = 0,
                .offset = 0
            }, {
                .buffer_slot = 0,
                .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
                .location = 1,
                .offset = sizeof(float) * 3
            }}
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
    };

    pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);
    if (!pipeline) {
        LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
        return;
    }

    SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

    vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * 3
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vertex) * 3
        }
    );

    Vertex* vertices = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    vertices[0] = (Vertex) { -1, -1, 0, 255, 0, 0, 255 };
    vertices[1] = (Vertex) { 1, -1, 0, 0, 255, 0, 255 };
    vertices[2] = (Vertex) { 0, 1, 0, 0, 0, 255, 255 };

    SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_GPUTransferBufferLocation source = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };
    SDL_GPUBufferRegion destination = {
        .buffer = vertex_buffer,
        .offset = 0,
        .size = sizeof(Vertex) * 3
    };

    SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

    LOG_INFO("Game window created");
}


void destroy_game_window() {
    SDL_DestroyWindow(app.window);
    app.window = NULL;

    SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipeline);
    pipeline = NULL;
    SDL_ReleaseGPUBuffer(app.gpu_device, vertex_buffer);
    vertex_buffer = NULL;

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
    LOG_INFO("Start draw");

    SDL_GPUCommandBuffer* gpu_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    if (!gpu_command_buffer) {
        LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
        return;
    }

    SDL_GPUTexture* swapchain_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(gpu_command_buffer, app.window, &swapchain_texture, NULL, NULL);

    if (swapchain_texture) {
        SDL_GPUColorTargetInfo color_target_info = {
            .texture = swapchain_texture,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }
        };

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(gpu_command_buffer, &color_target_info, 1, NULL);

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding) { .buffer = vertex_buffer, .offset = 0 }, 1);
        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(gpu_command_buffer);

    LOG_INFO("End draw");
}


void play_audio() {
    static float music_fade = 0.0f;
    static int current_music = -1;

    switch(app.state) {
        case STATE_GAME:
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
