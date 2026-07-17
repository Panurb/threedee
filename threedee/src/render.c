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


#define BLOOM_DOWNSAMPLE 4

static int frame_index = 0;
SDL_GPUFence* fences[FRAMES_IN_FLIGHT] = { 0 };

static CameraData camera_data[FRAMES_IN_FLIGHT];

static SDL_GPUTexture* depth_stencil_texture = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* shadow_maps[FRAMES_IN_FLIGHT] = { 0 };

static SDL_GPUTexture* screen_texture = NULL;
static SDL_GPUTexture* resolve_texture = NULL;
static SDL_GPUTexture* final_texture = NULL;
static SDL_GPUTexture* dof_temp_texture = NULL;
static SDL_GPUTexture* bloom_temp_textures[2] = { 0 };

static SDL_GPUSampler* screen_sampler = NULL;

static MultiBuffer light_buffer;
static ShadowUniformData shadow_map_data[MAX_LIGHTS] = { 0 };

static Mesh triangle_mesh;
static Mesh triangle_2d_mesh;
static Mesh quad_mesh;
static Mesh line_mesh;
static ArrayList* texts = NULL;

static Batch triangle_batch;
static Batch triangle_2d_batch;
static Batch quad_batch;
static Batch particle_batch;
static Batch particle_emissive_batch;
static Batch line_batch;
static Batch dummy_batch;
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
	final_texture = SDL_CreateGPUTexture(app.gpu_device, &resolve_texture_info);

	SDL_GPUTextureCreateInfo bloom_texture_info = {
		.width = game_settings.width / BLOOM_DOWNSAMPLE,
		.height = game_settings.height / BLOOM_DOWNSAMPLE,
		.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.layer_count_or_depth = 1,
		.num_levels = 1
	};
	for (int i = 0; i < 2; i++) {
		bloom_temp_textures[i] = SDL_CreateGPUTexture(app.gpu_device, &bloom_texture_info);
	}
}


MultiBuffer create_multi_buffer(int element_size, int capacity) {
	MultiBuffer multi_buffer = {
		.size = 0,
		.capacity = capacity,
		.element_size = element_size
	};

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		multi_buffer.buffer[i] = SDL_CreateGPUBuffer(
			app.gpu_device,
			&(SDL_GPUBufferCreateInfo){
				.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
				.size = element_size * capacity,
			}
		);

		multi_buffer.transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = element_size * capacity,
			}
		);

		multi_buffer.data[i] = NULL;
	}

	return multi_buffer;
}


void* get_multi_buffer_data(MultiBuffer* multi_buffer) {
	if (!multi_buffer->data[frame_index]) {
		multi_buffer->data[frame_index] = SDL_MapGPUTransferBuffer(
			app.gpu_device,
			multi_buffer->transfer_buffer[frame_index],
			false
		);
	}

	return multi_buffer->data[frame_index];
}


void upload_multi_buffer(SDL_GPUCopyPass* copy_pass, MultiBuffer* multi_buffer) {
	if (multi_buffer->data[frame_index]) {
		SDL_UnmapGPUTransferBuffer(
			app.gpu_device,
			multi_buffer->transfer_buffer[frame_index]
		);
		multi_buffer->data[frame_index] = NULL;
	}

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = multi_buffer->transfer_buffer[frame_index],
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = multi_buffer->buffer[frame_index],
			.offset = 0,
			.size = multi_buffer->element_size * multi_buffer->size
		},
		false
	);
}


Batch create_batch(Mesh* mesh, int instance_size) {
	Batch batch = {
		.mesh = mesh,
		.instances = create_multi_buffer(instance_size, 256),
	};
	batch.instances.resizable = true;

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

	light_buffer = create_multi_buffer(sizeof(LightData), MAX_LIGHTS);

	triangle_mesh = create_mesh_triangle();
	triangle_2d_mesh = create_mesh_triangle_2d();
	quad_mesh = create_mesh_quad();
	line_mesh = create_mesh_line();
	texts = ArrayList_create(sizeof(TextData));
	models = ArrayList_create(sizeof(Model));

	triangle_batch = create_batch(&triangle_mesh, sizeof(InstanceColorData));
	triangle_2d_batch = create_batch(&triangle_2d_mesh, sizeof(InstanceColorData2D));
	quad_batch = create_batch(&quad_mesh, sizeof(BillboardInstanceData));
	particle_batch = create_batch(&quad_mesh, sizeof(ParticleInstanceData));
	particle_emissive_batch = create_batch(&quad_mesh, sizeof(ParticleInstanceData));
	line_batch = create_batch(&line_mesh, sizeof(LineInstanceData));
	dummy_batch = create_batch(NULL, sizeof(InstanceData));

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

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		shadow_maps[i] = SDL_CreateGPUTexture(
			app.gpu_device,
			&(SDL_GPUTextureCreateInfo){
				.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
				.format = DEPTH_FORMAT,
				.width = SHADOW_MAP_RESOLUTION,
				.height = SHADOW_MAP_RESOLUTION,
				.layer_count_or_depth = MAX_LIGHTS,
				.num_levels = 1,
				.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			}
		);
	}

	create_screen_textures();
}


void destroy_multi_buffer(MultiBuffer* multi_buffer) {
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		if (multi_buffer->data[i]) {
			SDL_UnmapGPUTransferBuffer(
				app.gpu_device,
				multi_buffer->transfer_buffer[i]
			);
			multi_buffer->data[i] = NULL;
		}

		SDL_ReleaseGPUBuffer(app.gpu_device, multi_buffer->buffer[i]);
		SDL_ReleaseGPUTransferBuffer(app.gpu_device, multi_buffer->transfer_buffer[i]);
	}
}


void destroy_mesh(Mesh* mesh) {
	if (!mesh) return;

	SDL_ReleaseGPUBuffer(app.gpu_device, mesh->vertex_buffer);
	if (mesh->index_buffer) {
		SDL_ReleaseGPUBuffer(app.gpu_device, mesh->index_buffer);
	}
	// if (mesh->texture) {
	// 	SDL_ReleaseGPUTexture(app.gpu_device, mesh->texture);
	// }
}


void destroy_batch(Batch* batch) {
	if (!batch) return;

	destroy_mesh(batch->mesh);
	destroy_multi_buffer(&batch->instances);
}


void destroy_text_data(TextData* text_data) {
	if (!text_data) return;

	destroy_mesh(&text_data->mesh);
}


void apply_render_settings() {
	// Needs to be called if resolution, antialiasing settings change
	destroy_pipelines();
	create_pipelines();

	SDL_ReleaseGPUTexture(app.gpu_device, depth_stencil_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, screen_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, resolve_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, dof_temp_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, final_texture);
	for (int i = 0; i < 2; i++) {
		SDL_ReleaseGPUTexture(app.gpu_device, bloom_temp_textures[i]);
	}
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
					.texture = shadow_maps[frame_index],
					.sampler = sampler,
				},
				{
					.texture = resources.emissive_map_array,
					.sampler = sampler,
				}
			},
			4
		);

		SDL_BindGPUFragmentStorageBuffers(
			render_pass,
			0,
			(SDL_GPUBuffer*[]) {
				resources.materials_buffer,
				light_buffer.buffer[frame_index]
			},
			2
		);
	}

	if (pipeline == PIPELINE_PARTICLE_EMISSIVE || pipeline == PIPELINE_PARTICLE) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = resources.particle_array,
				.sampler = sampler,
			},
			1
		);
	}

	if (pipeline == PIPELINE_SHADOW_DEPTH) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = resources.texture_array,
				.sampler = sampler,
			},
			1
		);
	}
}


void render_mesh(
	SDL_GPURenderPass* render_pass,
	Mesh* mesh,
	int num_instances,
	int first_instance
) {
	if (mesh->vertex_buffer) {
		SDL_BindGPUVertexBuffers(
			render_pass,
			0,
			&(SDL_GPUBufferBinding) {
				.buffer = mesh->vertex_buffer,
				.offset = 0
			},
			1
		);
	}

	if (mesh->texture) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = mesh->texture,
				.sampler = sampler,
			},
			1
		);
	}

	if (mesh->index_buffer) {
		SDL_BindGPUIndexBuffer(
			render_pass,
			&(SDL_GPUBufferBinding) {
				.buffer = mesh->index_buffer,
				.offset = 0
			},
			SDL_GPU_INDEXELEMENTSIZE_16BIT
		);

		SDL_DrawGPUIndexedPrimitives(
			render_pass,
			mesh->num_indices,
			num_instances,
			0,
			0,
			first_instance
		);
	} else {
		SDL_DrawGPUPrimitives(
			render_pass,
			mesh->num_vertices,
			num_instances,
			0,
			first_instance
		);
	}
}


void init_batch_rendering(SDL_GPUCommandBuffer* command_buffer) {
	ModelUniformData model_uniform_data = {
		.use_instance_buffer = true
	};

	SDL_PushGPUVertexUniformData(
		command_buffer,
		1,
		&model_uniform_data,
		sizeof(ModelUniformData)
	);
}


void render_batch(SDL_GPURenderPass* render_pass, Batch* batch) {
	if (batch->instances.size == 0) {
		return;
	}

	if (batch->instances.element_size == 0) {
		LOG_ERROR("Instance size is zero");
	}

	if (batch->instances.data[frame_index]) {
		LOG_DEBUG("Batch %s has instance data still mapped, unmapping now", batch->mesh->name);
		SDL_UnmapGPUTransferBuffer(app.gpu_device, batch->instances.transfer_buffer[frame_index]);
		batch->instances.data[frame_index] = NULL;
	}

	SDL_BindGPUVertexStorageBuffers(
		render_pass,
		0,
		&batch->instances.buffer[frame_index],
		1
	);

	render_mesh(
		render_pass,
		batch->mesh,
		batch->instances.size,
		0
	);
}


void init_model_rendering(SDL_GPURenderPass* render_pass) {
	SDL_BindGPUVertexStorageBuffers(
		render_pass,
		0,
		&dummy_batch.instances.buffer[frame_index],
		1
	);
}


void render_model(
	SDL_GPUCommandBuffer* command_buffer,
	SDL_GPURenderPass* render_pass,
	Model* model
) {
	LOG_DEBUG("Rendering model: %s", model->mesh->name);
	ModelUniformData model_uniform_data = {
		.use_instance_buffer = false,
		.instance_data = model->instance_data
	};

	SDL_PushGPUVertexUniformData(
		command_buffer,
		1,
		&model_uniform_data,
		sizeof(ModelUniformData)
	);

	render_mesh(render_pass,model->mesh, 1, 0);
}


void add_light(
	Matrix4 transform,
	Color diffuse_color,
	Color specular_color,
	float fov,
	float range,
	Visibility visibility_mask,
	Matrix4 projection_matrix
) {
	Matrix4 view_matrix = inverse_transform(transform);
	Matrix4 projection_view_matrix = matrix4_mul(projection_matrix, view_matrix);

	LightData light_data = {
		.position = position_from_transform(transform),
		.visibility_mask = visibility_mask,
		.direction = quaternion_forward(rotation_from_transform(transform)),
		.cutoff_cos = cosf(to_radians(fov * 0.5f)),
		.diffuse_color = { diffuse_color.r, diffuse_color.g, diffuse_color.b },
		.specular_color = { specular_color.r, specular_color.g, specular_color.b },
		.projection_view_matrix = transpose4(projection_view_matrix),
		.range = range,
	};

	ShadowUniformData shadow_uniform_data = {
		.projection_view_matrix = light_data.projection_view_matrix,
		.visibility_mask = visibility_mask
	};

	LightData* data = get_multi_buffer_data(&light_buffer);
	data[light_buffer.size] = light_data;
	shadow_map_data[light_buffer.size] = shadow_uniform_data;
	light_buffer.size++;
}


void render_shadow_maps(SDL_GPUCommandBuffer* command_buffer) {
	for (int i = 0; i < light_buffer.size; i++) {
		ShadowUniformData shadow_uniform_data = shadow_map_data[i];

		SDL_PushGPUVertexUniformData(
			command_buffer,
			0,
			&shadow_uniform_data,
			sizeof(ShadowUniformData)
		);

		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
			command_buffer,
			 NULL,
			0,
			&(SDL_GPUDepthStencilTargetInfo){
				.clear_depth = 1.0f,
				.texture = shadow_maps[frame_index],
				.cycle = false,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
				.stencil_store_op = SDL_GPU_STOREOP_STORE,
				.layer = i,
			}
		);

		bind_pipeline(render_pass, PIPELINE_SHADOW_DEPTH);

		init_model_rendering(render_pass);
		for (int j = 0; j < models->size; j++) {
			Model* model = ArrayList_get(models, j);
			render_model(command_buffer, render_pass, model);
		}

		init_batch_rendering(command_buffer);
		for (int j = 0; j < resources.meshes_size; j++) {
			render_batch(render_pass, &batches[j]);
		}

		SDL_EndGPURenderPass(render_pass);
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
		.focal_distance = (camera->focal_distance - camera->near_plane) / (camera->far_plane - camera->near_plane),
		.focal_range = camera->focal_range,
		.vertical = vertical
	};
	SDL_PushGPUFragmentUniformData(command_buffer, 1, &dof_uniform_data, sizeof(DepthOfFieldUniformData));
	SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(render_pass);
}


void render_bloom_phase(
	SDL_GPUCommandBuffer* command_buffer,
	SDL_GPUTexture* source,
	SDL_GPUTexture* target,
	Pipeline pipeline
) {
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
	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[pipeline]);
	SDL_BindGPUFragmentSamplers(
		render_pass,
		0,
		&(SDL_GPUTextureSamplerBinding){
			.texture = source,
			.sampler = screen_sampler,
		},
		1
	);
	SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(render_pass);
}


void render_bloom_combine(
	SDL_GPUCommandBuffer* command_buffer,
	SDL_GPUTexture* source1,
	SDL_GPUTexture* source2,
	SDL_GPUTexture* target
) {
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
	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[PIPELINE_BLOOM_COMBINE]);
	SDL_BindGPUFragmentSamplers(
		render_pass,
		0,
		(SDL_GPUTextureSamplerBinding[]){
			{
				.texture = source1,
				.sampler = screen_sampler,
			},
			{
				.texture = source2,
				.sampler = screen_sampler,
			}
		},
		2
	);
	SDL_DrawGPUPrimitives(render_pass, 4, 1, 0, 0);

	SDL_EndGPURenderPass(render_pass);
}


void render_bloom(
	SDL_GPUCommandBuffer* command_buffer,
	SDL_GPUTexture* source,
	SDL_GPUTexture* target
) {
	BloomUniformData bloom_uniform_data = {
		.bloom_threshold = scene->bloom.threshold,
		.bloom_knee = scene->bloom.knee,
		.bloom_intensity = scene->bloom.intensity,
		.bloom_strength = scene->bloom.strength,
		.texel_size = {
			1.0f / (float)(game_settings.width / BLOOM_DOWNSAMPLE),
			1.0f / (float)(game_settings.height / BLOOM_DOWNSAMPLE)
		},
		.vertical = false
	};
	SDL_PushGPUFragmentUniformData(
		command_buffer,
		1,
		&bloom_uniform_data,
		sizeof(BloomUniformData)
	);

	render_bloom_phase(command_buffer, source, bloom_temp_textures[0], PIPELINE_BLOOM_EXTRACT);
	render_bloom_phase(command_buffer, bloom_temp_textures[0], bloom_temp_textures[1], PIPELINE_BLOOM_BLUR);

	bloom_uniform_data.vertical = true;
	SDL_PushGPUFragmentUniformData(
		command_buffer,
		1,
		&bloom_uniform_data,
		sizeof(BloomUniformData)
	);

	render_bloom_phase(command_buffer, bloom_temp_textures[1], bloom_temp_textures[0], PIPELINE_BLOOM_BLUR);
	render_bloom_combine(command_buffer, source, bloom_temp_textures[0], target);
}


void set_camera_data(Matrix4 projection_matrix, Matrix4 view_matrix, Vector3 position) {
	camera_data[frame_index] = (CameraData) {
		.projection_matrix = transpose4(projection_matrix),
		.view_matrix = transpose4(view_matrix),
		.position = position
	};
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
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

		upload_multi_buffer(copy_pass, &light_buffer);

		for (int i = 0; i < resources.meshes_size; i++) {
			upload_multi_buffer(copy_pass, &batches[i].instances);
		}
		upload_multi_buffer(copy_pass, &triangle_batch.instances);
		upload_multi_buffer(copy_pass, &quad_batch.instances);
		upload_multi_buffer(copy_pass, &line_batch.instances);
		upload_multi_buffer(copy_pass, &triangle_2d_batch.instances);

		upload_multi_buffer(copy_pass, &particle_batch.instances);
		upload_multi_buffer(copy_pass, &particle_emissive_batch.instances);

		SDL_EndGPUCopyPass(copy_pass);

		render_shadow_maps(command_buffer);

		SDL_PushGPUVertexUniformData(
			command_buffer,
			0,
			&camera_data[frame_index],
			sizeof(CameraData)
		);

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
				.cycle = false,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
				.stencil_store_op = SDL_GPU_STOREOP_STORE,
			}
		);

		WeatherComponent* weather = get_component(scene->weather, COMPONENT_WEATHER);

		CameraComponent* camera = get_component(scene->camera, COMPONENT_CAMERA);

		UniformData uniform_data = {
			.near_plane = camera->near_plane,
			.far_plane = camera->far_plane,
			.ambient_light = weather->ambient_light,
			.num_lights = light_buffer.size,
			.camera_position = camera_data[frame_index].position,
			.shadow_map_resolution = SHADOW_MAP_RESOLUTION,
			.fog_color = weather->fog_color,
			.fog_start = weather->fog_start,
			.fog_end = weather->fog_end,
		};
		SDL_PushGPUFragmentUniformData(command_buffer, 0, &uniform_data, sizeof(UniformData));

		bind_pipeline(render_pass, PIPELINE_3D_TEXTURED);
		init_model_rendering(render_pass);
		for (int i = 0; i < models->size; i++) {
			Model* model = ArrayList_get(models, i);
			render_model(command_buffer, render_pass, model);
		}

		init_batch_rendering(command_buffer);
		for (int i = 0; i < resources.meshes_size; i++) {
			render_batch(render_pass, &batches[i]);
		}

		bind_pipeline(render_pass, PIPELINE_BILLBOARD);
		render_batch(render_pass, &quad_batch);

		bind_pipeline(render_pass, PIPELINE_PARTICLE);
		render_batch(render_pass, &particle_batch);

		bind_pipeline(render_pass, PIPELINE_PARTICLE_EMISSIVE);
		render_batch(render_pass, &particle_emissive_batch);

		bind_pipeline(render_pass, PIPELINE_3D);
		render_batch(render_pass, &triangle_batch);

		bind_pipeline(render_pass, PIPELINE_LINE);
		render_batch(render_pass, &line_batch);

		SDL_EndGPURenderPass(render_pass);

		SDL_PushGPUFragmentUniformData(
			command_buffer,
			0,
			&(PostProcessingUniformData){
				.near_plane = camera->near_plane,
				.far_plane = camera->far_plane,
				.screen_size = { (float)game_settings.width, (float)game_settings.height }
			},
			sizeof(PostProcessingUniformData)
		);

		SDL_GPUTexture* source_texture = game_settings.antialiasing == 0 ? screen_texture : resolve_texture;

		if (game_settings.bloom) {
			render_bloom(command_buffer, source_texture, final_texture);
		} else {
			final_texture = source_texture;
		}

		if (camera->dof_enabled) {
			render_depth_of_field(command_buffer, final_texture, dof_temp_texture, false);
			render_depth_of_field(command_buffer, dof_temp_texture, final_texture, true);
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
					.texture = final_texture,
					.sampler = screen_sampler,
				},
				{
					.texture = depth_stencil_texture,
					.sampler = screen_sampler,
				}
			},
			2
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
		render_batch(render_pass, &triangle_2d_batch);

		bind_pipeline(render_pass, PIPELINE_TEXT);
		for (int i = 0; i < texts->size; i++) {
			TextData* text = ArrayList_get(texts, i);

			SDL_PushGPUVertexUniformData(
				command_buffer,
				1,
				&text->instance_color_data,
				sizeof(InstanceColorData2D)
			);

			render_mesh(render_pass, &text->mesh, 1, 0);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	fences[frame_index] = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);

	LOG_DEBUG("Submitted frame %d", frame_index);

	// Reset instance counts for next frame
	light_buffer.size = 0;
	for (int i = 0; i < resources.meshes_size; i++) {
		batches[i].instances.size = 0;
	}
	ArrayList_for_each(texts, destroy_text_data);
	ArrayList_clear(texts);
	triangle_batch.instances.size = 0;
	triangle_2d_batch.instances.size = 0;
	quad_batch.instances.size = 0;
	line_batch.instances.size = 0;
	particle_batch.instances.size = 0;
	particle_emissive_batch.instances.size = 0;

	frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;
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


void double_multi_buffer_size(MultiBuffer* multi_buffer) {
	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);

	int size = multi_buffer->element_size * multi_buffer->capacity;

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		multi_buffer->buffer[i] = double_buffer_size(
			command_buffer,
			multi_buffer->buffer[i],
			size
		);

		SDL_GPUTransferBuffer* new_transfer_buffer = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 2 * size,
			}
		);

		// Only need to copy buffer data for the current frame
		if (i == frame_index) {
			LOG_DEBUG("Copying instance transfer buffer data for batch %s, frame %d", batch->mesh->name, i);
			SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

			void* data = get_multi_buffer_data(multi_buffer);
			multi_buffer->data[i] = SDL_MapGPUTransferBuffer(app.gpu_device, new_transfer_buffer, false);

			SDL_memcpy(multi_buffer->data[i], data, size);

			// Unmap old transfer buffer, keep new one mapped
			SDL_UnmapGPUTransferBuffer(app.gpu_device, multi_buffer->transfer_buffer[i]);

			SDL_EndGPUCopyPass(copy_pass);
		}

		SDL_ReleaseGPUTransferBuffer(app.gpu_device, multi_buffer->transfer_buffer[i]);
		multi_buffer->transfer_buffer[i] = new_transfer_buffer;
	}

	multi_buffer->capacity *= 2;
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


void check_multi_buffer_size(MultiBuffer* multi_buffer) {
	if (!multi_buffer->resizable) {
		LOG_ERROR("Multi buffer size is not resizable");
		return;
	}

	if (multi_buffer->size < multi_buffer->capacity) {
		return;
	}
	LOG_INFO("MultiBuffer full, resizing...");

	wait_for_fences();
	double_multi_buffer_size(multi_buffer);

	LOG_INFO("New buffer size: %d", multi_buffer->capacity);
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
	check_multi_buffer_size(&batch->instances);

	InstanceData* transforms = get_multi_buffer_data(&batch->instances);

	InstanceData instance_data = {
		.transform = transpose4(transform),
		.material_index = material_index,
		.texture_index = texture_index,
		.emissive_index = emissive_index,
		.texture_scale = texture_scale,
		.visibility = visibility,
		.emissive = emissive
	};
	transforms[batch->instances.size] = instance_data;
	batch->instances.size++;
}


CubeIndices CubeIndices_fill(int value) {
	CubeIndices indices = {
		.front = value,
		.back = value,
		.left = value,
		.right = value,
		.top = value,
		.bottom = value
	};
	return indices;
}


void draw_sprite(Vector3 position, float width, float height, int texture_index) {
	LOG_DEBUG("Drawing sprite with texture %d", texture_index);

	Batch* batch = &quad_batch;
	check_multi_buffer_size(&batch->instances);

	BillboardInstanceData* instances = get_multi_buffer_data(&batch->instances);

	BillboardInstanceData instance_data = {
		.position = position,
		.width = width,
		.height = height,
		.texture_index = texture_index,
		.material_index = 1,
		.visiblity = VISIBILITY_ALL,
		.type = BILLBOARD_CYLINDRICAL
	};
	instances[batch->instances.size] = instance_data;
	batch->instances.size++;
}


void draw_particle(Vector3 position, float width, float height, float angle, int texture_index, Color color, bool emissive) {
	LOG_DEBUG("Drawing particle with texture %d", texture_index);

	Batch* batch = emissive ? &particle_emissive_batch : &particle_batch;
	check_multi_buffer_size(&batch->instances);

	ParticleInstanceData* instances = get_multi_buffer_data(&batch->instances);

	ParticleInstanceData instance_data = {
		.color = color,
		.position = position,
		.width = width,
		.height = height,
		.angle = angle,
		.texture_index = texture_index,
		.visiblity = VISIBILITY_ALL,
	};
	instances[batch->instances.size] = instance_data;
	batch->instances.size++;
}


void draw_triangle(Vector3 a, Vector3 b, Vector3 c, Color color) {
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
	check_multi_buffer_size(&batch->instances);

	InstanceColorData* instance_datas = get_multi_buffer_data(&batch->instances);
	InstanceColorData instance_data = {
		.transform = transpose4(transform),
		.color = color
	};
	instance_datas[batch->instances.size] = instance_data;
	batch->instances.size++;
}


void draw_line(Vector3 start, Vector3 end, float thickness, Color color) {
	LOG_DEBUG("Drawing batched line from (%f, %f, %f) to (%f, %f, %f)", start.x, start.y, start.z, end.x, end.y, end.z);

	Batch* batch = &line_batch;
	check_multi_buffer_size(&batch->instances);

	LineInstanceData* instance_datas = get_multi_buffer_data(&batch->instances);

	LineInstanceData instance_data = {
		.start = start,
		.end = end,
		.thickness = thickness,
		.color = color
	};
	instance_datas[batch->instances.size] = instance_data;
	batch->instances.size++;
}


void draw_circle(Vector3 center, float radius, int segments, Color color) {
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
		draw_triangle(center, prev_point, current_point, color);
		prev_point = current_point;
	}
}


void draw_sphere(Vector3 center, float radius, int segments, Color color) {
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

			draw_quad(a, b, c, d, color);
		}
	}
}


void draw_cuboid(Vector3 position, Quaternion rotation, Vector3 size, Color color) {
	Matrix4 transform = transform_matrix(position, rotation, size);

	Vector4 corners[8] = {
		{-0.5f, -0.5f, -0.5f, 1.0f},
		{ 0.5f, -0.5f, -0.5f, 1.0f},
		{ 0.5f,  0.5f, -0.5f, 1.0f},
		{-0.5f,  0.5f, -0.5f, 1.0f},
		{-0.5f, -0.5f,  0.5f, 1.0f},
		{ 0.5f, -0.5f,  0.5f, 1.0f},
		{ 0.5f,  0.5f,  0.5f, 1.0f},
		{-0.5f,  0.5f,  0.5f, 1.0f},
	};

	int quads[6][4] = {
		{0, 1, 2, 3}, // Back
		{5, 4, 7, 6}, // Front
		{4, 0, 3, 7}, // Left
		{1, 5, 6, 2}, // Right
		{3, 2, 6, 7}, // Top
		{4, 5, 1, 0}  // Bottom
	};

	for (int i = 0; i < 8; i++) {
		corners[i] = map4(transform, corners[i]);
	}

	for (int i = 0; i < 6; i++) {
		Vector3 a = vec4_xyz(corners[quads[i][0]]);
		Vector3 b = vec4_xyz(corners[quads[i][1]]);
		Vector3 c = vec4_xyz(corners[quads[i][2]]);
		Vector3 d = vec4_xyz(corners[quads[i][3]]);
		draw_quad(a, b, c, d, color);
	}
}


void draw_quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color) {
	draw_triangle(a, b, c, color);
	draw_triangle(a, c, d, color);
}


void draw_arrow(Vector3 start, Vector3 end, float thickness, Color color) {
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

	draw_triangle(end, left, right, color);
}


void draw_plane(Plane plane, Color color) {
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

	draw_quad(a, b, c, d, color);
}


void draw_triangle_2d(Vector2 a, Vector2 b, Vector2 c, Color color) {
	Batch* batch = &triangle_2d_batch;
	check_multi_buffer_size(&batch->instances);

	InstanceColorData2D* instance_datas = get_multi_buffer_data(&batch->instances);
	InstanceColorData2D instance_data = {
		.transform = {
			b.x - a.x, c.x - a.x, a.x, 0.0f,
			b.y - a.y, c.y - a.y, a.y, 0.0f,
		},
		.color = color
	};
	instance_datas[batch->instances.size] = instance_data;
	batch->instances.size++;
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

	// TODO: do I have to free this data?
	TTF_GPUAtlasDrawSequence* data = TTF_GetGPUTextDrawData(text);

	if (data->next != NULL) {
		LOG_WARNING("Text %s has more than one draw sequence, only the first will be rendered", string);
	}

	Mesh mesh = create_mesh_text(*data);

	// Match text pixel size to screen coordinates
	float scale = size / 216.0f;

	InstanceColorData2D instance_data = {
		.transform = {
			scale * cosf(angle), -scale * sinf(angle), position.x, 0.0f,
			scale * sinf(angle), scale * cosf(angle), position.y, 0.0f
		},
		.color = color
	};

	TextData text_data = {
		.mesh = mesh,
		.instance_color_data = instance_data,
	};
	ArrayList_add(texts, &text_data);
	TTF_DestroyText(text);
}
