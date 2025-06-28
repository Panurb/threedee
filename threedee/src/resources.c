#include <render.h>
#include <stdio.h>
#include <SDL3_image/SDL_image.h>

#include "util.h"
#include "app.h"
#include "resources.h"
#include "arraylist.h"


typedef struct {
	int position_idx;
	int normal_idx;
	int uv_idx;
} VertexIndices;


SDL_GPUTexture* create_texture(SDL_Surface* image_data) {
	int num_levels = floorf(log2f(fmaxf(image_data->w, image_data->h)) + 1);
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


SDL_GPUTexture* create_texture_array(SDL_Surface** images, int num_images) {
	SDL_Surface* first_image = images[0];

	int num_levels = floorf(log2f(fmaxf(first_image->w, first_image->h)) + 1);
	SDL_GPUTexture* texture = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.width = first_image->w,
			.height = first_image->h,
			.layer_count_or_depth = num_images,
			.num_levels = num_levels,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		}
	);
	if (!texture) {
		LOG_ERROR("Failed to create texture: %s", SDL_GetError());
	}

	int image_size = first_image->w * first_image->h * 4;
	SDL_GPUTransferBuffer* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo) {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = image_size * num_images
		}
	);

	Uint8* texture_transfer_ptr = SDL_MapGPUTransferBuffer(
		app.gpu_device,
		texture_transfer_buffer,
		false
	);

	SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

	for (int i = 0; i < num_images; i++) {
		SDL_Surface* image_data = images[i];
		SDL_memcpy(texture_transfer_ptr + (i * image_size), image_data->pixels, image_size);

		SDL_UploadToGPUTexture(
			copy_pass,
			&(SDL_GPUTextureTransferInfo) {
				.transfer_buffer = texture_transfer_buffer,
				.offset = i * image_size,
			},
			&(SDL_GPUTextureRegion){
				.texture = texture,
				.w = image_data->w,
				.h = image_data->h,
				.d = 1,
				.layer = i
			},
			false
		);
	}

	SDL_UnmapGPUTransferBuffer(app.gpu_device, texture_transfer_buffer);

	SDL_EndGPUCopyPass(copy_pass);

	SDL_GenerateMipmapsForGPUTexture(upload_command_buffer, texture);

	SDL_SubmitGPUCommandBuffer(upload_command_buffer);

	SDL_ReleaseGPUTransferBuffer(app.gpu_device, texture_transfer_buffer);

	return texture;
}


Vector3 calculate_tangent(Vector3 v0, Vector3 v1, Vector3 v2, Vector2 uv0, Vector2 uv1, Vector2 uv2) {
	Vector3 edge1 = diff3(v1, v0);
	Vector3 edge2 = diff3(v2, v0);
	Vector2 delta_uv1 = diff2(uv1, uv0);
	Vector2 delta_uv2 = diff2(uv2, uv0);

	float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
	Vector3 tangent;
	tangent.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
	tangent.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
	tangent.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

	return normalized3(tangent);
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
		.num_instances = 0,
		.instance_size = sizeof(InstanceData),
	};
	strcpy(mesh_data.name, path);

	ArrayList* unique_vertices = ArrayList_create(sizeof(VertexIndices));
	ArrayList* positions = ArrayList_create(sizeof(Vector3));
	ArrayList* normals = ArrayList_create(sizeof(Vector3));
	ArrayList* uvs = ArrayList_create(sizeof(Vector2));
	ArrayList* indices = ArrayList_create(sizeof(Uint16));

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
			ArrayList_add(positions, &position);
		} else if (line[0] == 'f') {
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
				VertexIndices vi = {
					.position_idx = v[i] - 1,
					.normal_idx = vn[i] - 1,
					.uv_idx = vt[i] - 1
				};

				int vertex_index = ArrayList_find(unique_vertices, &vi);
				if (vertex_index == -1) {
					ArrayList_add(unique_vertices, &vi);
					vertex_index = unique_vertices->size - 1;
				}
				ArrayList_add(indices, &vertex_index);
			}
		}
	}

	mesh_data.num_vertices = unique_vertices->size;
	mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices,
		}
	);

	mesh_data.num_indices = indices->size;
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

	for (int i = 0; i < unique_vertices->size; i++) {
		VertexIndices vi = *(VertexIndices*)ArrayList_get(unique_vertices, i);
		PositionTextureVertex ptv = {
			.position = *(Vector3*)ArrayList_get(positions, vi.position_idx),
			.uv = *(Vector2*)ArrayList_get(uvs, vi.uv_idx),
			.normal = *(Vector3*)ArrayList_get(normals, vi.normal_idx),
			.tangent = zeros3()
		};
		transfer_data[i] = ptv;
	}

	for (int i = 0; i < indices->size; i++) {
		int index = *(int*)ArrayList_get(indices, i);
		index_data[i] = (Uint16)index;
	}

	for (int i = 0; i < indices->size; i += 3) {
		Uint16 i0 = index_data[i];
		Uint16 i1 = index_data[i + 1];
		Uint16 i2 = index_data[i + 2];

		Vector3 u0 = transfer_data[i0].position;
		Vector3 u1 = transfer_data[i1].position;
		Vector3 u2 = transfer_data[i2].position;
		Vector2 uv0 = transfer_data[i0].uv;
		Vector2 uv1 = transfer_data[i1].uv;
		Vector2 uv2 = transfer_data[i2].uv;
		Vector3 tangent = calculate_tangent(
			u0, u1, u2,
			uv0, uv1, uv2
		);
		transfer_data[i0].tangent = normalized3(sum3(transfer_data[i0].tangent, tangent));
		transfer_data[i1].tangent = normalized3(sum3(transfer_data[i1].tangent, tangent));
		transfer_data[i2].tangent = normalized3(sum3(transfer_data[i2].tangent, tangent));
	}

	ArrayList_destroy(unique_vertices);
	ArrayList_destroy(positions);
	ArrayList_destroy(normals);
	ArrayList_destroy(uvs);
	ArrayList_destroy(indices);
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



	return mesh_data;
}


void load_textures() {
	resources.textures_size = list_files_alphabetically("data/images/*.png", resources.texture_names);
	ArrayList* surfaces = ArrayList_create(sizeof(SDL_Surface*));

	for (int i = 0; i < resources.textures_size; i++) {
		LOG_INFO("Loading texture: %s", resources.texture_names[i]);
		String path;
		snprintf(path, STRING_SIZE, "%s%s%s", "data/images/", resources.texture_names[i], ".png");
		SDL_Surface* surface = IMG_Load(path);
		LOG_INFO("Format: %s, Size: %dx%d", SDL_GetPixelFormatName(surface->format), surface->w, surface->h);
		if (!surface) {
			LOG_ERROR("Failed to load image: %s", path);
			continue;
		}
		ArrayList_add(surfaces, &surface);
	}

	resources.texture_array = create_texture_array(surfaces->data, surfaces->size);

	ArrayList_for_each(surfaces, SDL_DestroySurface);
	ArrayList_destroy(surfaces);
}


void load_normal_maps() {
	ArrayList* surfaces = ArrayList_create(sizeof(SDL_Surface*));

	for (int i = 0; i < resources.textures_size; i++) {
		String path;
		snprintf(path, STRING_SIZE, "%s%s%s", "data/normals/", resources.texture_names[i], ".png");
		SDL_Surface* surface = IMG_Load(path);
		if (!surface) {
			LOG_WARNING("Missing normal map: %s", path);
			// Add a default normal map (flat blue = no perturbation)
			surface = SDL_CreateSurface(2048, 2048, SDL_PIXELFORMAT_RGBA8888);
			SDL_FillSurfaceRect(surface, NULL, 0x8080FF);
		}
		ArrayList_add(surfaces, &surface);
	}

	resources.normal_map_array = create_texture_array(surfaces->data, surfaces->size);

	ArrayList_for_each(surfaces, SDL_DestroySurface);
	ArrayList_destroy(surfaces);
}


void load_resources() {
    LOG_INFO("Loading resources");

	resources = (Resources) { 0 };

	load_textures();
	// load_normal_maps();

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
