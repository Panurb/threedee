#include "resources.h"

#include <stdio.h>
#include <SDL3_image/SDL_image.h>

#include "util.h"
#include "app.h"


SDL_GPUTexture* load_texture(String path) {
	LOG_INFO("Loading texture: %s", path);
    SDL_Surface* image_data = IMG_Load(path);
	if (!image_data) {
		LOG_ERROR("Failed to load image data: %s", SDL_GetError());
	}

	SDL_GPUTexture* texture = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.width = image_data->w,
			.height = image_data->h,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		}
	);
	if (!texture) {
		LOG_ERROR("Failed to create texture: %s", SDL_GetError());
	}

	SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo) {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = image_data->w * image_data->h * 4
		}
	);

	Uint8* texture_transfer_ptr = SDL_MapGPUTransferBuffer(
		app.gpu_device,
		texture_transfer_buffer,
		false
	);
		SDL_memcpy(texture_transfer_ptr, image_data->pixels, image_data->w * image_data->h * 4);
		SDL_UnmapGPUTransferBuffer(app.gpu_device, texture_transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

	SDL_UploadToGPUTexture(
		copy_pass,
		&(SDL_GPUTextureTransferInfo) {
			.transfer_buffer = texture_transfer_buffer,
			.offset = 0, /* Zeros out the rest */
		},
		&(SDL_GPUTextureRegion){
			.texture = texture,
			.w = image_data->w,
			.h = image_data->h,
			.d = 1
		},
		false
	);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
	SDL_DestroySurface(image_data);

	return texture;
}


void load_resources() {
    LOG_INFO("Loading resources");

    resources.textures_size = list_files_alphabetically("data/images/*.png", resources.texture_names);
    for (int i = 0; i < resources.textures_size; i++) {
        String path;
        snprintf(path, STRING_SIZE, "%s%s%s", "data/images/", resources.texture_names[i], ".png");
        resources.textures[i] = load_texture(path);
        if (!resources.textures[i]) {
            LOG_ERROR("Failed to load texture: %s", path);
        }
    }

    resources.sounds_size = list_files_alphabetically("data/sfx/*.wav", resources.sound_names);
    for (int i = 0; i < resources.sounds_size; i++) {
        String path;
        snprintf(path, STRING_SIZE, "%s%s%s", "data/sfx/", resources.sound_names[i], ".wav");
        resources.sounds[i] = Mix_LoadWAV(path);
        if (!resources.sounds[i]) {
            LOG_ERROR("Failed to load sound: %s", path);
        }
    }

    LOG_INFO("Resources loaded");
}
