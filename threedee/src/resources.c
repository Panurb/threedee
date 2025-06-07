#include <render.h>
#include <stdio.h>
#include <SDL3_image/SDL_image.h>

#include "util.h"
#include "app.h"
#include "resources.h"
#include "arraylist.h"


SDL_GPUTexture* create_texture(SDL_Surface* image_data) {
	int num_levels = floorf(log2f(fmaxf(image_data->w, image_data->h)) + 1);
	LOG_INFO("Number of levels: %d", num_levels);
	SDL_GPUTexture* texture = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.width = image_data->w,
			.height = image_data->h,
			.layer_count_or_depth = 1,
			.num_levels = num_levels,
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

	SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, texture);

    SDL_SubmitGPUCommandBuffer(upload_command_buffer);

	SDL_ReleaseGPUTransferBuffer(app.gpu_device, texture_transfer_buffer);

	return texture;
}


MeshData load_mesh(String path) {
	LOG_INFO("Loading mesh: %s", path);

	FILE* file = fopen(path, "rb");
	if (!file) {
		LOG_ERROR("Failed to open mesh file: %s", path);
		return (MeshData){0};
	}

	MeshData mesh_data = {
		.max_instances = 128,
		.num_instances = 0
	};

	ArrayList* vertices = ArrayList_create(sizeof(Vector3));
	ArrayList* normals = ArrayList_create(sizeof(Vector3));
	ArrayList* uvs = ArrayList_create(sizeof(Vector2));

	String line;
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == '#') continue;

		if (strncmp(line, "vn", 2) == 0) {
			Vector3 normal;
			int matches = sscanf(line, "vn %f %f %f", &normal.x, &normal.y, &normal.z);
			if (matches != 3) {
				LOG_ERROR("Invalid normal format in line: %s", line);
				continue;
			}
			ArrayList_add(normals, &normal);
		} else if (strncmp(line, "vt", 2) == 0) {
			Vector2 uv;
			int matches = sscanf(line, "vt %f %f", &uv.x, &uv.y);
			if (matches != 2) {
				LOG_ERROR("Invalid UV format in line: %s", line);
				continue;
			}
			ArrayList_add(uvs, &uv);
		} else if (line[0] == 'v') {
			Vector3 position;
			int matches = sscanf(line, "v %f %f %f", &position.x, &position.y, &position.z);
			if (matches != 3) {
				LOG_ERROR("Invalid vertex format in line: %s", line);
				continue;
			}
			ArrayList_add(vertices, &position);
		} else if (line[0] == 'f') {
			mesh_data.num_indices += 3;
		}
	}

	mesh_data.num_vertices = vertices->size;
	mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices,
		}
	);

	mesh_data.index_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * mesh_data.num_indices,
		}
	);

	mesh_data.instance_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
			.size = sizeof(InstanceData) * mesh_data.max_instances,
		}
	);

	mesh_data.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = sizeof(InstanceData) * mesh_data.max_instances,
		}
	);

	SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
		}
	);

	PositionTextureVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);
	Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];

	rewind(file);

	int triangle_index = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] == 'f') {
			int v[3], vt[3], vn[3];
			int matches = sscanf(
				line,
				"f %d/%d/%d %d/%d/%d %d/%d/%d",
				&v[0], &vt[0], &vn[0],
				&v[1], &vt[1], &vn[1],
				&v[2], &vt[2], &vn[2]
			);
			if (matches != 9) {
				LOG_ERROR("Invalid face format in line: %s", line);
				continue;
			}

			for (int i = 0; i < 3; i++) {
				transfer_data[v[i] - 1] = (PositionTextureVertex) {
					.position = *(Vector3*)ArrayList_get(vertices, v[i] - 1),
					.uv = *(Vector2*)ArrayList_get(uvs, vt[i] - 1),
					.normal = *(Vector3*)ArrayList_get(normals, vn[i] - 1)
				};
			}
			index_data[3 * triangle_index] = v[0] - 1;
			index_data[3 * triangle_index + 1] = v[1] - 1;
			index_data[3 * triangle_index + 2] = v[2] - 1;
			triangle_index++;
		}
	}

	ArrayList_destroy(vertices);
	ArrayList_destroy(normals);
	ArrayList_destroy(uvs);
	fclose(file);

	SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

	SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data.vertex_buffer,
			.offset = 0,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices
		},
		false
	);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex) * mesh_data.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh_data.num_indices
		},
		false
	);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_command_buffer);
	SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	mesh_data.sampler = SDL_CreateGPUSampler(
		app.gpu_device,
		&(SDL_GPUSamplerCreateInfo){
			.min_filter = SDL_GPU_FILTER_NEAREST,
			.mag_filter = SDL_GPU_FILTER_NEAREST,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.enable_anisotropy = true,
			.max_anisotropy = 16,
		}
	);

	return mesh_data;
}


void load_textures() {
	resources.textures_size = list_files_alphabetically("data/images/*.png", resources.texture_names);
	ArrayList* surfaces = ArrayList_create(sizeof(SDL_Surface*));
	int atlas_width = 0;
	int atlas_height = 0;
	int padding = 16;
	for (int i = 0; i < resources.textures_size; i++) {
		String path;
		snprintf(path, STRING_SIZE, "%s%s%s", "data/images/", resources.texture_names[i], ".png");
		SDL_Surface* surface = IMG_Load(path);
		if (!surface) {
			LOG_ERROR("Failed to load image: %s", path);
			continue;
		}
		ArrayList_add(surfaces, &surface);

		atlas_width += surface->w + 2 * padding;
		if (surface->h > atlas_height) {
			atlas_height = surface->h;
		}
	}

	SDL_Surface* atlas = SDL_CreateSurface(atlas_width, atlas_height, SDL_PIXELFORMAT_RGBA32);
	int offset_x = padding;
	for (int i = 0; i < resources.textures_size; i++) {
		SDL_Surface* surface = *(SDL_Surface**)ArrayList_get(surfaces, i);
		for (int y = 0; y < padding; y++) {
			SDL_BlitSurface(
				surface,
				&(SDL_Rect){ 0, 0, 1, surface->h },
				atlas,
				&(SDL_Rect){ offset_x - y - 1, 0 }
			);
			SDL_BlitSurface(
				surface,
				&(SDL_Rect){ surface->w - 1, 0, 1, surface->h },
				atlas,
				&(SDL_Rect){ offset_x + surface->w + y, 0 }
			);
		}
		SDL_BlitSurface(
			surface,
			NULL,
			atlas,
			&(SDL_Rect){ offset_x, 0 }
		);
		resources.texture_rects[i] = (Rect) {
			.x = offset_x / (float)atlas->w,
			.y = 0.0f,
			.w = surface->w / (float)atlas->w,
			.h = surface->h / (float)atlas->h
		};
		offset_x += surface->w + 2 * padding;
	}

	// IMG_SavePNG(atlas, "data/images/atlas.png");

	ArrayList_for_each(surfaces, SDL_DestroySurface);
	ArrayList_destroy(surfaces);

	resources.texture_atlas = create_texture(atlas);
	SDL_DestroySurface(atlas);
}


void load_resources() {
    LOG_INFO("Loading resources");

	resources = (Resources) { 0 };

	load_textures();

    resources.sounds_size = list_files_alphabetically("data/sfx/*.wav", resources.sound_names);
    for (int i = 0; i < resources.sounds_size; i++) {
        String path;
        snprintf(path, STRING_SIZE, "%s%s%s", "data/sfx/", resources.sound_names[i], ".wav");
        resources.sounds[i] = Mix_LoadWAV(path);
        if (!resources.sounds[i]) {
            LOG_ERROR("Failed to load sound: %s", path);
        }
    }

	resources.meshes_size = list_files_alphabetically("data/meshes/*.obj", resources.mesh_names);
	for (int i = 0; i < resources.meshes_size; i++) {
		String path;
		snprintf(path, STRING_SIZE, "%s%s%s", "data/meshes/", resources.mesh_names[i], ".obj");
		resources.meshes[i] = load_mesh(path);
		if (!resources.meshes[i].vertex_buffer) {
			LOG_ERROR("Failed to load mesh: %s", path);
		}
	}

	resources.materials[0] = (Material) {0.3f, 0.3f, 0.3f, 8.0f};
	strcpy(resources.material_names[0], "concrete");
	resources.materials[1] = (Material) {0.5f, 0.5f, 0.5f, 32.0f};
	strcpy(resources.material_names[1], "default");
	resources.materials[2] = (Material) {0.9f, 0.9f, 0.9f, 64.0f};
	strcpy(resources.material_names[2], "glass");
	resources.materials[3] = (Material) {0.1f, 0.1f, 0.1f, 16.0f};
	strcpy(resources.material_names[3], "plastic");
	resources.materials_size = 4;

    LOG_INFO("Resources loaded");
}
