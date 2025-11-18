#define _USE_MATH_DEFINES

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"
#include "render_pipeline.h"
#include "render_mesh.h"
#include "component.h"
#include "resources.h"
#include "scene.h"
#include "settings.h"
#include "app.h"
#include "util.h"


static int frame_index = 0;
SDL_GPUFence* fences[FRAMES_IN_FLIGHT] = { 0 };

static SDL_GPUTexture* depth_stencil_texture = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* shadow_maps = NULL;
static SDL_GPUTexture* screen_texture = NULL;
static SDL_GPUTexture* resolve_texture = NULL;
static SDL_GPUTexture* dof_temp_texture = NULL;
static SDL_GPUSampler* screen_sampler = NULL;

static LightData lights[MAX_LIGHTS];
static int num_lights = 0;

static MeshData triangle_mesh;
static MeshData triangle_2d_mesh;
static MeshData quad_mesh;
static MeshData line_mesh;
static ArrayList* texts;

static Batch triangle_batch;
static Batch triangle_2d_batch;
static Batch quad_batch;
static Batch line_batch;

static Batch batches[MAX_MESHES] = { 0 };


SDL_GPUSampleCount get_sample_count() {
	switch (game_settings.antialiasing) {
		case 0: return SDL_GPU_SAMPLECOUNT_1;
		case 2: return SDL_GPU_SAMPLECOUNT_2;
		case 4: return SDL_GPU_SAMPLECOUNT_4;
		case 8: return SDL_GPU_SAMPLECOUNT_8;
		default: return SDL_GPU_SAMPLECOUNT_1;
	}
}


void create_screen_textures() {
	SDL_GPUTextureCreateInfo depth_stencil_texture_info = {
		.width = game_settings.width,
		.height = game_settings.height,
		.format = DEPTH_FORMAT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = get_sample_count()
	};
	depth_stencil_texture = SDL_CreateGPUTexture(app.gpu_device, &depth_stencil_texture_info);

	SDL_GPUTextureCreateInfo screen_texture_info = {
		.width = game_settings.width,
		.height = game_settings.height,
		.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
		.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = get_sample_count()
	};
	if (game_settings.antialiasing == 0) {
		screen_texture_info.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;
	}
	screen_texture = SDL_CreateGPUTexture(
		app.gpu_device,
		&screen_texture_info
	);

	SDL_GPUTextureCreateInfo resolve_texture_info = {
		.width = game_settings.width,
		.height = game_settings.height,
		.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.layer_count_or_depth = 1,
		.num_levels = 1
	};
	resolve_texture = SDL_CreateGPUTexture(app.gpu_device, &resolve_texture_info);

	dof_temp_texture = SDL_CreateGPUTexture(app.gpu_device, &resolve_texture_info);
}


Batch create_batch(MeshData* mesh, int instance_size) {
	Batch batch = {
		.mesh = mesh,
		.instance_size = instance_size,
		.num_instances = 0,
		.max_instances = 256,
	};

	batch.instance_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
			.size = batch.instance_size * batch.max_instances * FRAMES_IN_FLIGHT,
		}
	);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		batch.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = batch.instance_size * batch.max_instances,
			}
		);
	}

	return batch;
}


void init_render() {
	SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

	SDL_SetGPUSwapchainParameters(
		app.gpu_device,
		app.window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		game_settings.vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE
	);

	load_shaders();
	create_pipelines();

	triangle_mesh = create_mesh_triangle();
	triangle_2d_mesh = create_mesh_triangle_2d();
	quad_mesh = create_mesh_quad();
	line_mesh = create_mesh_line();
	texts = ArrayList_create(sizeof(TextData));

	triangle_batch = create_batch(&triangle_mesh, sizeof(InstanceColorData));
	triangle_2d_batch = create_batch(&triangle_2d_mesh, sizeof(InstanceColorData2D));
	quad_batch = create_batch(&quad_mesh, sizeof(BillboardInstanceData));
	line_batch = create_batch(&line_mesh, sizeof(LineInstanceData));

	for (int i = 0; i < resources.meshes_size; i++) {
		batches[i] = create_batch(&resources.meshes[i], sizeof(InstanceData));
	}

	sampler = SDL_CreateGPUSampler(
		app.gpu_device,
		&(SDL_GPUSamplerCreateInfo){
			.min_filter = SDL_GPU_FILTER_LINEAR,
			.mag_filter = SDL_GPU_FILTER_LINEAR,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.enable_anisotropy = true,
			.max_anisotropy = (float)game_settings.anisotropic_filtering,
			.min_lod = 0.0f,
			.max_lod = 1000.0f
		}
	);

	screen_sampler = SDL_CreateGPUSampler(
		app.gpu_device,
		&(SDL_GPUSamplerCreateInfo){
			.min_filter = SDL_GPU_FILTER_LINEAR,
			.mag_filter = SDL_GPU_FILTER_LINEAR,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		}
	);

	shadow_maps = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
			.format = DEPTH_FORMAT,
			.width = SHADOW_MAP_RESOLUTION,
			.height = SHADOW_MAP_RESOLUTION,
			.layer_count_or_depth = MAX_LIGHTS,
			.num_levels = 1,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		}
	);

	create_screen_textures();
}


void destroy_mesh(MeshData* mesh_data) {
	if (!mesh_data) return;

	SDL_ReleaseGPUBuffer(app.gpu_device, mesh_data->vertex_buffer);
	if (mesh_data->index_buffer) {
		SDL_ReleaseGPUBuffer(app.gpu_device, mesh_data->index_buffer);
	}
	// if (mesh_data->texture) {
	// 	SDL_ReleaseGPUTexture(app.gpu_device, mesh_data->texture);
	// }
}


void destroy_batch(Batch* batch) {
	if (!batch) return;

	SDL_ReleaseGPUBuffer(app.gpu_device, batch->instance_buffer);
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		SDL_ReleaseGPUTransferBuffer(app.gpu_device, batch->instance_transfer_buffer[i]);
	}
}


void destroy_text_data(TextData* text_data) {
	if (!text_data) return;

	destroy_mesh(&text_data->mesh);
	destroy_batch(&text_data->batch);
	TTF_DestroyText(text_data->text);
}


void apply_render_settings() {
	// Needs to be called if resolution, antialiasing settings change
	destroy_pipelines();
	create_pipelines();

	SDL_ReleaseGPUTexture(app.gpu_device, depth_stencil_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, screen_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, resolve_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, dof_temp_texture);
	create_screen_textures();
}


void bind_pipeline(SDL_GPURenderPass* render_pass, Pipeline pipeline) {
	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[pipeline]);

	if (pipeline == PIPELINE_3D_TEXTURED) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			(SDL_GPUTextureSamplerBinding[]) {
				{
					.texture = resources.texture_array,
					.sampler = sampler,
				},
				{
					.texture = resources.normal_map_array,
					.sampler = sampler,
				},
				{
					.texture = shadow_maps,
					.sampler = sampler,
				},
				{
					.texture = resources.emissive_map_array,
					.sampler = sampler,
				}
			},
			4
		);
	}
}


void render_batch(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass, Batch* batch) {
	if (batch->num_instances == 0) {
		return;
	}

	if (batch->instance_size == 0) {
		LOG_ERROR("Instance size is zero");
	}

	if (batch->instance_data[frame_index]) {
		LOG_DEBUG("Batch %s has instance data still mapped, unmapping now", mesh_data->name);
		SDL_UnmapGPUTransferBuffer(app.gpu_device, batch->instance_transfer_buffer[frame_index]);
		batch->instance_data[frame_index] = NULL;
	}

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(gpu_command_buffer);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = batch->instance_transfer_buffer[frame_index],
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = batch->instance_buffer,
			.offset = frame_index * batch->max_instances * batch->instance_size,
			.size = batch->instance_size * batch->num_instances
		},
		true
	);

	SDL_EndGPUCopyPass(copy_pass);

	SDL_BindGPUVertexStorageBuffers(render_pass, 0, &batch->instance_buffer, 1);

	MeshData* mesh_data = batch->mesh;

	if (mesh_data->vertex_buffer) {
		SDL_BindGPUVertexBuffers(
			render_pass,
			0,
			&(SDL_GPUBufferBinding) {
				.buffer = mesh_data->vertex_buffer,
				.offset = 0
			},
			1
		);
	}

	if (mesh_data->texture) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = mesh_data->texture,
				.sampler = sampler,
			},
			1
		);
	}

	if (mesh_data->index_buffer) {
		SDL_BindGPUIndexBuffer(
			render_pass,
			&(SDL_GPUBufferBinding) {
				.buffer = mesh_data->index_buffer,
				.offset = 0
			},
			SDL_GPU_INDEXELEMENTSIZE_16BIT
		);

		SDL_DrawGPUIndexedPrimitives(
			render_pass,
			mesh_data->num_indices,
			batch->num_instances,
			0,
			0,
			frame_index * batch->max_instances
		);
	} else {
		SDL_DrawGPUPrimitives(
			render_pass,
			mesh_data->num_vertices,
			batch->num_instances,
			0,
			frame_index * batch->max_instances
		);
	}
}


void add_light(Entity entity) {
	LightComponent* light = get_component(entity, COMPONENT_LIGHT);
	Color diffuse_color = light->diffuse_color;
	Color specular_color = light->specular_color;

	LightData light_data = {
		.position = get_position(entity),
		.visibility_mask = light->visibility_mask,
		.direction = quaternion_forward(get_rotation(entity)),
		.cutoff_cos = cosf(to_radians(light->fov * 0.5f)),
		.diffuse_color = { diffuse_color.r, diffuse_color.g, diffuse_color.b },
		.specular_color = { specular_color.r, specular_color.g, specular_color.b },
		.projection_view_matrix = transpose4(light->shadow_map.projection_view_matrix),
		.range = light->range,
	};

	memcpy(lights + num_lights, &light_data, sizeof(LightData));
	num_lights++;
}


void render_shadow_maps(SDL_GPUCommandBuffer* command_buffer) {
	for (Entity i = 0; i < scene->components->entities; i++) {
		LightComponent* light = get_component(i, COMPONENT_LIGHT);
		if (!light) continue;
		if (light->disabled) continue;

		ShadowUniformData shadow_uniform_data = {
			.projection_view_matrix = transpose4(light->shadow_map.projection_view_matrix),
			.visibility_mask = light->visibility_mask
		};
		SDL_PushGPUVertexUniformData(command_buffer, 0, &shadow_uniform_data, sizeof(ShadowUniformData));

		if (!light->shadow_map.depth_texture) {
			LOG_ERROR("Light %d does not have a shadow map depth texture!", i);
		}

		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
			command_buffer,
			 NULL,
			0,
			&(SDL_GPUDepthStencilTargetInfo){
				.clear_depth = 1.0f,
				.texture = light->shadow_map.depth_texture,
				.cycle = true,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
				.stencil_store_op = SDL_GPU_STOREOP_STORE,
			}
		);

		bind_pipeline(render_pass, PIPELINE_SHADOW_DEPTH);
		for (int j = 0; j < resources.meshes_size; j++) {
			render_batch(command_buffer, render_pass, &batches[j]);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	int layer = 0;
	for (Entity i = 0; i < scene->components->entities; i++) {
		LightComponent* light = get_component(i, COMPONENT_LIGHT);
		if (!light) continue;
		if (light->disabled) continue;

		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
		SDL_CopyGPUTextureToTexture(
			copy_pass,
			&(SDL_GPUTextureLocation) {
				.texture = light->shadow_map.depth_texture,
				.layer = 0
			},
			&(SDL_GPUTextureLocation) {
				.texture = shadow_maps,
				.layer = layer,
			},
			SHADOW_MAP_RESOLUTION,
			SHADOW_MAP_RESOLUTION,
			1,
			false
		);
		SDL_EndGPUCopyPass(copy_pass);
		layer++;
	}
}


void render_depth_of_field(SDL_GPUCommandBuffer* command_buffer, SDL_GPUTexture* source, SDL_GPUTexture* target, bool vertical) {
	SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
		command_buffer,
		&(SDL_GPUColorTargetInfo) {
			.texture = target,
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.store_op = SDL_GPU_STOREOP_STORE
		},
		1,
		NULL
	);
	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[PIPELINE_DEPTH_OF_FIELD]);
	SDL_BindGPUFragmentSamplers(
		render_pass,
		0,
		(SDL_GPUTextureSamplerBinding[]){
			{
				.texture = source,
				.sampler = screen_sampler,
			},
			{
				.texture = depth_stencil_texture,
				.sampler = screen_sampler,
			}
		},
		2
	);

	CameraComponent* camera = get_component(scene->camera, COMPONENT_CAMERA);
	DepthOfFieldUniformData dof_uniform_data = {
		.near_plane = camera->near_plane,
		.far_plane = camera->far_plane,
		.focal_distance = (camera->focal_distance - camera->near_plane) / (camera->far_plane - camera->near_plane),
		.focal_range = camera->focal_range,
		.screen_size = { (float)game_settings.width, (float)game_settings.height },
		.vertical = vertical
	};
	SDL_PushGPUFragmentUniformData(command_buffer, 0, &dof_uniform_data, sizeof(DepthOfFieldUniformData));
	SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(render_pass);
}


void pre_render() {
	LOG_DEBUG("Pre-rendering frame %d", frame_index);

	if (fences[frame_index]) {
		LOG_DEBUG("Waiting for GPU fence for frame %d", frame_index);
		SDL_WaitForGPUFences(app.gpu_device, true, &fences[frame_index], 1);
		SDL_ReleaseGPUFence(app.gpu_device, fences[frame_index]);
		fences[frame_index] = NULL;
	}
}


void render() {
	LOG_DEBUG("Rendering frame %d", frame_index);

	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
	if (!command_buffer) {
		LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
		return;
	}

	SDL_GPUTexture* swapchain_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window, &swapchain_texture, NULL, NULL);

	if (swapchain_texture) {
		render_shadow_maps(command_buffer);

		CameraComponent* camera = get_component(scene->camera, COMPONENT_CAMERA);
		Matrix4 view_matrix = inverse_transform(get_transform(scene->camera));
		CameraData camera_data = {
			.projection_matrix = transpose4(camera->projection_matrix),
			.view_matrix = transpose4(view_matrix),
			.position = get_position(scene->camera),
		};

		SDL_PushGPUVertexUniformData(command_buffer, 0, &camera_data, sizeof(CameraData));

		SDL_GPUColorTargetInfo color_target_info = {
			.texture = screen_texture,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.clear_color = {
				.r = COLOR_SKY.r,
				.g = COLOR_SKY.g,
				.b = COLOR_SKY.b,
				.a = 1.0f
			},
		};

		if (game_settings.antialiasing != 0) {
			color_target_info.store_op = SDL_GPU_STOREOP_RESOLVE;
			color_target_info.resolve_texture = resolve_texture;
		}

		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
			command_buffer,
			&color_target_info,
			1,
			&(SDL_GPUDepthStencilTargetInfo){
				.clear_depth = 1.0f,
				.texture = depth_stencil_texture,
				.cycle = true,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
				.stencil_store_op = SDL_GPU_STOREOP_STORE,
			}
		);

		WeatherComponent* weather = get_component(scene->weather, COMPONENT_WEATHER);

		UniformData uniform_data = {
			.near_plane = camera->near_plane,
			.far_plane = camera->far_plane,
			.ambient_light = weather->ambient_light,
			.num_lights = num_lights,
			.camera_position = get_position(scene->camera),
			.shadow_map_resolution = SHADOW_MAP_RESOLUTION,
			.fog_color = weather->fog_color,
			.fog_start = weather->fog_start,
			.fog_end = weather->fog_end,
		};
		SDL_PushGPUFragmentUniformData(command_buffer, 0, &uniform_data, sizeof(UniformData));
		SDL_PushGPUFragmentUniformData(command_buffer, 1, &lights, sizeof(LightData) * num_lights);

		SDL_BindGPUFragmentStorageBuffers(
			render_pass,
			0,
			&resources.materials_buffer,
			1
		);

		bind_pipeline(render_pass, PIPELINE_3D_TEXTURED);
		for (int i = 0; i < resources.meshes_size; i++) {
			render_batch(command_buffer, render_pass, &batches[i]);
		}

		bind_pipeline(render_pass, PIPELINE_3D);
		render_batch(command_buffer, render_pass, &triangle_batch);

		bind_pipeline(render_pass, PIPELINE_BILLBOARD);
		render_batch(command_buffer, render_pass, &quad_batch);

		bind_pipeline(render_pass, PIPELINE_LINE);
		render_batch(command_buffer, render_pass, &line_batch);

		SDL_EndGPURenderPass(render_pass);

		SDL_GPUTexture* source_texture = game_settings.antialiasing == 0 ? screen_texture : resolve_texture;
		if (camera->dof_enabled) {
			render_depth_of_field(command_buffer, source_texture, dof_temp_texture, false);
			render_depth_of_field(command_buffer, dof_temp_texture, source_texture, true);
		}

		// Draw to swapchain texture
		color_target_info = (SDL_GPUColorTargetInfo) {
			.texture = swapchain_texture,
			.load_op = SDL_GPU_LOADOP_DONT_CARE,
			.store_op = SDL_GPU_STOREOP_STORE
		};

		render_pass = SDL_BeginGPURenderPass(
			command_buffer,
			&color_target_info,
			1,
			NULL
		);
		SDL_BindGPUGraphicsPipeline(render_pass, pipelines[PIPELINE_POST_PROCESSING]);
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			(SDL_GPUTextureSamplerBinding[]) {
				{
					.texture = source_texture,
					.sampler = screen_sampler,
				},
				{
					.texture = depth_stencil_texture,
					.sampler = screen_sampler,
				}
			},
			2
		);
		SDL_PushGPUFragmentUniformData(
			command_buffer,
			0,
			&(PostProcessingUniformData){
				.near_plane = camera->near_plane,
				.far_plane = camera->far_plane,
			},
			sizeof(PostProcessingUniformData)
		);
		SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

		SDL_EndGPURenderPass(render_pass);

		render_pass = SDL_BeginGPURenderPass(
			command_buffer,
			&(SDL_GPUColorTargetInfo) {
				.texture = swapchain_texture,
				.load_op = SDL_GPU_LOADOP_LOAD,
				.store_op = SDL_GPU_STOREOP_STORE
			},
			1,
			NULL
		);

		CameraComponent* screen_camera = get_component(scene->screen_camera, COMPONENT_CAMERA);
		Matrix4 projection_matrix = transpose4(screen_camera->projection_matrix);
		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_matrix, sizeof(Matrix4));

		bind_pipeline(render_pass, PIPELINE_2D);
		render_batch(command_buffer, render_pass, &triangle_2d_batch);

		bind_pipeline(render_pass, PIPELINE_TEXT);
		for (int i = 0; i < texts->size; i++) {
			TextData* text = ArrayList_get(texts, i);
			Batch batch = text->batch;
			batch.mesh = &text->mesh;
			render_batch(command_buffer, render_pass, &batch);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	fences[frame_index] = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);

	LOG_DEBUG("Submitted frame %d", frame_index);

	// Reset instance counts for next frame
	num_lights = 0;
	for (int i = 0; i < resources.meshes_size; i++) {
		batches[i].num_instances = 0;
	}
	ArrayList_for_each(texts, destroy_text_data);
	ArrayList_clear(texts);
	triangle_batch.num_instances = 0;
	triangle_2d_batch.num_instances = 0;
	quad_batch.num_instances = 0;
	line_batch.num_instances = 0;

	frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;
}


void* get_batch_instance_data(Batch* batch) {
	if (batch->instance_data[frame_index]) {
		return batch->instance_data[frame_index];
	}

	LOG_DEBUG("Mapping instance data for batch %d, frame %d", batch->mesh_index, frame_index);
	batch->instance_data[frame_index] = SDL_MapGPUTransferBuffer(
		app.gpu_device, batch->instance_transfer_buffer[frame_index], false
	);
	return batch->instance_data[frame_index];
}

SDL_GPUBuffer* double_buffer_size(SDL_GPUCommandBuffer* command_buffer, SDL_GPUBuffer* buffer, int size) {
	LOG_DEBUG("Doubling buffer size from %d to %d", size, 2 * size);
	SDL_GPUBuffer* new_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
			.size = 2 * size,
		}
	);

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
	SDL_CopyGPUBufferToBuffer(
		copy_pass,
		&(SDL_GPUBufferLocation) {
			.buffer = buffer,
			.offset = 0
		},
		&(SDL_GPUBufferLocation) {
			.buffer = new_buffer,
			.offset = 0
		},
		size,
		false
	);
	SDL_EndGPUCopyPass(copy_pass);

	SDL_ReleaseGPUBuffer(app.gpu_device, buffer);

	LOG_DEBUG("Buffer size doubled successfully");

	return new_buffer;
}


void double_batch_buffer_sizes(Batch* batch) {
	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);

	int size = batch->instance_size * batch->max_instances;

	// Instance buffer
	batch->instance_buffer = double_buffer_size(
		command_buffer,
		batch->instance_buffer,
		size * FRAMES_IN_FLIGHT
	);

	// Instance transfer buffers
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		SDL_GPUTransferBuffer* new_transfer_buffer = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 2 * size,
			}
		);

		// Only need to copy buffer data for the current frame
		if (i == frame_index) {
			LOG_DEBUG("Copying instance transfer buffer data for batch %d, frame %d", batch->mesh_index, i);
			SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

			void* data = get_batch_instance_data(batch);
			batch->instance_data[i] = SDL_MapGPUTransferBuffer(app.gpu_device, new_transfer_buffer, false);

			SDL_memcpy(batch->instance_data[i], data, size);

			// Unmap old transfer buffer, keep new one mapped
			SDL_UnmapGPUTransferBuffer(app.gpu_device, batch->instance_transfer_buffer[i]);

			SDL_EndGPUCopyPass(copy_pass);
		}

		SDL_ReleaseGPUTransferBuffer(app.gpu_device, batch->instance_transfer_buffer[i]);
		batch->instance_transfer_buffer[i] = new_transfer_buffer;
	}

	batch->max_instances *= 2;
}


void wait_for_fences() {
	LOG_DEBUG("Waiting for GPU fences");
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		if (!fences[i]) continue;

		SDL_WaitForGPUFences(app.gpu_device, true, &fences[i], 1);
		SDL_ReleaseGPUFence(app.gpu_device, fences[i]);
		fences[i] = NULL;
	}
}


void check_batch_buffer_sizes(Batch* batch) {
	if (batch->num_instances < batch->max_instances) {
		return;
	}
	LOG_INFO("Buffer for mesh %s full, resizing...", batch->mesh->name);

	wait_for_fences();
	double_batch_buffer_sizes(batch);

	LOG_INFO("New buffer size: %d", batch->max_instances / batch->instance_size);
}


void draw_mesh(
	Matrix4 transform,
	int mesh_index,
	int texture_index,
	int material_index,
	int emissive_index,
	float emissive,
	Visibility visibility,
	Vector2 texture_scale
) {
	LOG_DEBUG("Drawing mesh %d with texture %d", mesh_index, texture_index);

	Batch* batch = &batches[mesh_index];
	check_batch_buffer_sizes(batch);

	InstanceData* transforms = get_batch_instance_data(batch);

	InstanceData instance_data = {
		.transform = transpose4(transform),
		.material_index = material_index,
		.texture_index = texture_index,
		.emissive_index = emissive_index,
		.texture_scale = texture_scale,
		.visiblity = visibility,
		.emissive = emissive
	};
	transforms[batch->num_instances] = instance_data;
	batch->num_instances++;
}


void draw_sprite(Vector3 position, float width, float height, int texture_index) {
	LOG_DEBUG("Drawing sprite with texture %d", texture_index);

	Batch* batch = &quad_batch;
	check_batch_buffer_sizes(batch);

	BillboardInstanceData* instances = get_batch_instance_data(batch);

	BillboardInstanceData instance_data = {
		.position = position,
		.width = width,
		.height = height,
		.texture_index = texture_index,
		.material_index = 1,
		.visiblity = VISIBILITY_ALL,
		.type = BILLBOARD_CYLINDRICAL
	};
	instances[batch->num_instances] = instance_data;
	batch->num_instances++;
}


void render_triangle(Vector3 a, Vector3 b, Vector3 c, Color color) {
	Vector3 n = cross(
		sub3(b, a),
		sub3(c, a)
	);
	Matrix4 transform = {
		b.x - a.x, c.x - a.x, n.x, a.x,
		b.y - a.y, c.y - a.y, n.y, a.y,
		b.z - a.z, c.z - a.z, n.z, a.z,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Batch* batch = &triangle_batch;
	check_batch_buffer_sizes(batch);

	InstanceColorData* instance_datas = get_batch_instance_data(batch);
	InstanceColorData instance_data = {
		.transform = transpose4(transform),
		.color = color
	};
	instance_datas[batch->num_instances] = instance_data;
	batch->num_instances++;
}


void draw_line(Vector3 start, Vector3 end, float thickness, Color color) {
	LOG_DEBUG("Drawing batched line from (%f, %f, %f) to (%f, %f, %f)", start.x, start.y, start.z, end.x, end.y, end.z);

	Batch* batch = &line_batch;
	check_batch_buffer_sizes(batch);

	LineInstanceData* instance_datas = get_batch_instance_data(batch);

	LineInstanceData instance_data = {
		.start = start,
		.end = end,
		.thickness = thickness,
		.color = color
	};
	instance_datas[batch->num_instances] = instance_data;
	batch->num_instances++;
}


void render_circle(Vector3 center, float radius, int segments, Color color) {
	if (segments < 3) return; // At least a triangle

	float angle_increment = 2.0f * M_PI / segments;
	Vector3 prev_point = {center.x + radius, center.y, center.z};

	for (int i = 1; i <= segments; i++) {
		float angle = i * angle_increment;
		Vector3 current_point = {
			center.x + radius * cosf(angle),
			center.y + radius * sinf(angle),
			center.z
		};
		render_triangle(center, prev_point, current_point, color);
		prev_point = current_point;
	}
}


void render_sphere(Vector3 center, float radius, int segments, Color color) {
	if (segments < 3) return; // At least a triangle

	float angle_increment = M_PI / segments;
	for (int i = 0; i < segments; i++) {
		float theta1 = i * angle_increment;
		float theta2 = (i + 1) * angle_increment;

		for (int j = 0; j < segments; j++) {
			float phi1 = j * (2.0f * M_PI / segments);
			float phi2 = (j + 1) * (2.0f * M_PI / segments);

			Vector3 a = {
				center.x + radius * sinf(theta1) * cosf(phi1),
				center.y + radius * sinf(theta1) * sinf(phi1),
				center.z + radius * cosf(theta1)
			};
			Vector3 b = {
				center.x + radius * sinf(theta1) * cosf(phi2),
				center.y + radius * sinf(theta1) * sinf(phi2),
				center.z + radius * cosf(theta1)
			};
			Vector3 c = {
				center.x + radius * sinf(theta2) * cosf(phi2),
				center.y + radius * sinf(theta2) * sinf(phi2),
				center.z + radius * cosf(theta2)
			};
			Vector3 d = {
				center.x + radius * sinf(theta2) * cosf(phi1),
				center.y + radius * sinf(theta2) * sinf(phi1),
				center.z + radius * cosf(theta2)
			};

			render_quad(a, b, c, d, color);
		}
	}
}


void render_quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color) {
	render_triangle(a, b, c, color);
	render_triangle(a, c, d, color);
}


void render_arrow(Vector3 start, Vector3 end, float thickness, Color color) {
	// Arrow tip size
	float tip_length = 4.0f * thickness;
	float tip_width = 5.0f * thickness;

	Vector3 direction = sub3(end, start);
	float len = norm3(direction);
	if (len < 1e-6f) return;
	Vector3 dir = normalized3(direction);

	draw_line(
		start,
		add3(start, mul3(fmaxf(len - tip_length, 0.0f), dir)),
		thickness,
		color
	);

	Vector3 up = {0.0f, 0.0f, 1.0f};
	if (fabsf(dot3(dir, up)) > 0.99f) {
		up = (Vector3){0.0f, 1.0f, 0.0f};
	}
	Vector3 perp = normalized3(cross(dir, up));

	Vector3 tip_base = sub3(end, mul3(tip_length, dir));
	Vector3 left = add3(tip_base, mul3(tip_width / 2.0f, perp));
	Vector3 right = sub3(tip_base, mul3(tip_width / 2.0f, perp));

	render_triangle(end, left, right, color);
}


void render_plane(Plane plane, Color color) {
	// Create a large quad in the plane's normal direction
	Vector3 up = {0.0f, 0.0f, 1.0f};
	if (fabsf(dot3(plane.normal, up)) > 0.99f) {
		up = (Vector3){0.0f, 1.0f, 0.0f}; // Use a different up if parallel
	}
	Vector3 right = normalized3(cross(plane.normal, up));
	Vector3 forward = normalized3(cross(right, plane.normal));

	float size = 100.0f; // Size of the plane
	Vector3 center = mul3(plane.offset, plane.normal);
	Vector3 a = add3(center, mul3(size, right));
	Vector3 b = add3(center, mul3(size, forward));
	Vector3 c = sub3(center, mul3(size, right));
	Vector3 d = sub3(center, mul3(size, forward));

	render_quad(a, b, c, d, color);
}


void draw_triangle_2d(Vector2 a, Vector2 b, Vector2 c, Color color) {
	Batch* batch = &triangle_2d_batch;
	check_batch_buffer_sizes(batch);

	InstanceColorData2D* instance_datas = get_batch_instance_data(batch);
	InstanceColorData2D instance_data = {
		.transform = {
			b.x - a.x, c.x - a.x, a.x, 0.0f,
			b.y - a.y, c.y - a.y, a.y, 0.0f,
		},
		.color = color
	};
	instance_datas[batch->num_instances] = instance_data;
	batch->num_instances++;
}


void draw_circle_2d(Vector2 center, float radius, Color color) {
	int segments = 1000 * radius;

	float angle_increment = 2.0f * M_PI / segments;
	Vector2 prev_point = {center.x + radius, center.y};

	for (int i = 1; i <= segments; i++) {
		float angle = i * angle_increment;
		Vector2 current_point = {
			center.x + radius * cosf(angle),
			center.y + radius * sinf(angle)
		};
		draw_triangle_2d(center, prev_point, current_point, color);
		prev_point = current_point;
	}
}


void draw_text(String string, Vector2 position, float angle, float size, Color color) {
	TTF_Text* text = TTF_CreateText(app.text_engine, resources.fonts[0], string, 0);

	TTF_GPUAtlasDrawSequence* data = TTF_GetGPUTextDrawData(text);

	if (data->next != NULL) {
		LOG_WARNING("Text %s has more than one draw sequence, only the first will be rendered", string);
	}

	MeshData mesh_data = create_mesh_text(*data);
	Batch batch = create_batch(NULL, sizeof(InstanceColorData2D));

	// Match text pixel size to screen coordinates
	float scale = size / 216.0f;

	InstanceColorData2D* instance_datas = SDL_MapGPUTransferBuffer(
		app.gpu_device, batch.instance_transfer_buffer[frame_index], false
	);
	InstanceColorData2D instance_data = {
		.transform = {
			scale * cosf(angle), -scale * sinf(angle), position.x, 0.0f,
			scale * sinf(angle), scale * cosf(angle), position.y, 0.0f
		},
		.color = color
	};
	instance_datas[batch.num_instances] = instance_data;
	batch.num_instances++;

	TextData text_data = {
		.mesh = mesh_data,
		.batch = batch,
		.text = text
	};
	ArrayList_add(texts, &text_data);
}
