#define _USE_MATH_DEFINES

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"

#include <camera.h>
#include <component.h>
#include <resources.h>
#include <scene.h>
#include <settings.h>
#include <SDL3_image/SDL_image.h>

#include "app.h"
#include "util.h"


typedef enum {
	PIPELINE_2D,
	PIPELINE_3D,
	PIPELINE_3D_TEXTURED,
	PIPELINE_SHADOW_DEPTH,
	PIPELINE_COUNT
} Pipeline;


static SDL_GPUGraphicsPipeline** pipelines;
static SDL_GPUCommandBuffer* command_buffer = NULL;
static SDL_GPUTexture* depth_stencil_texture = NULL;
static SDL_GPUSampler* sampler = NULL;
static SDL_GPUTexture* shadow_maps = NULL;

static LightData lights[MAX_LIGHTS];
static int num_lights = 0;

static MeshData triangle_mesh;


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

	char full_path[256];
	SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	const char *entry_point;

	if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
		SDL_snprintf(full_path, sizeof(full_path), "%sdata/shaders/compiled/SPIRV/%s.spv", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_SPIRV;
		entry_point = "main";
	} else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
		SDL_snprintf(full_path, sizeof(full_path), "%sdata/shaders/compiled/MSL/%s.msl", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_MSL;
		entry_point = "main0";
	} else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
		SDL_snprintf(full_path, sizeof(full_path), "%sdata/shaders/compiled/DXIL/%s.dxil", app.base_path, shader_filename);
		format = SDL_GPU_SHADERFORMAT_DXIL;
		entry_point = "main";
	} else {
		SDL_Log("%s", "Unrecognized backend shader format!");
		return NULL;
	}

	size_t code_size;
	void* code = SDL_LoadFile(full_path, &code_size);
	if (code == NULL)
	{
		SDL_Log("Failed to load shader from disk! %s", full_path);
		return NULL;
	}

	SDL_GPUShaderCreateInfo shader_info = {
		.code = code,
		.code_size = code_size,
		.entrypoint = entry_point,
		.format = format,
		.stage = stage,
		.num_samplers = sampler_count,
		.num_uniform_buffers = uniform_buffer_count,
		.num_storage_buffers = storage_buffer_count,
		.num_storage_textures = storage_texture_count
	};
	SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
	if (shader == NULL)
	{
		SDL_Log("Failed to create shader!");
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_2d() {
	SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "position_color.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "solid_color.frag", 0, 0, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionColorVertex)
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

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_3d() {
	SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "position_color.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "solid_color_depth.frag", 0, 1, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = (SDL_GPUColorTargetBlendState) {
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.enable_blend = true
				}
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(Vector3)
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
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_3d_textured() {
	SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "position_texture.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "phong.frag", 2, 2, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionTextureVertex)
			}},
			.num_vertex_attributes = 4,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 3
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 2,
				.offset = sizeof(float) * 5
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 3,
				.offset = sizeof(float) * 8
			}}
		},
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_shadow_depth() {
	SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "shadow_depth.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "shadow_depth.frag", 0, 0, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 0,
			.color_target_descriptions = NULL,
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionTextureVertex)
			}},
			.num_vertex_attributes = 4,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 3
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 2,
				.offset = sizeof(float) * 5
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 3,
				.offset = sizeof(float) * 8
			}}
		},
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


MeshData create_mesh_triangle() {
	MeshData mesh_data = {
		.name = "triangle",
		.max_instances = 256,
		.num_instances = 0,
		.instance_size = sizeof(InstanceColorData),
	};

	mesh_data.num_vertices = 3;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector3) * mesh_data.num_vertices,
        }
    );

    mesh_data.num_indices = 3;
    mesh_data.index_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * mesh_data.num_indices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector3) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
        }
    );

    mesh_data.instance_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(InstanceColorData) * mesh_data.max_instances,
        }
    );

    mesh_data.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(InstanceColorData) * mesh_data.max_instances,
        }
    );

    Vector3* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vector3) { 0.0f, 0.0f, 0.0f };
	transfer_data[1] = (Vector3) { 1.0f, 0.0f, 0.0f };
	transfer_data[2] = (Vector3) { 0.0f, 1.0f, 0.0f };

    Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
    const Uint16 indices[3] = { 0, 1, 2 };
    SDL_memcpy(index_data, indices, sizeof(indices));

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
            .size = sizeof(Vector3) * mesh_data.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(Vector3) * mesh_data.num_vertices
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


void init_render() {
	app.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

	pipelines = malloc(sizeof(SDL_GPUGraphicsPipeline*) * PIPELINE_COUNT);
	pipelines[PIPELINE_2D] = create_render_pipeline_2d();
	pipelines[PIPELINE_3D] = create_render_pipeline_3d();
	pipelines[PIPELINE_3D_TEXTURED] = create_render_pipeline_3d_textured();
	pipelines[PIPELINE_SHADOW_DEPTH] = create_render_pipeline_shadow_depth();

	triangle_mesh = create_mesh_triangle();

	SDL_GPUTextureCreateInfo depth_stencil_texture_info = {
		.width = game_settings.width,
		.height = game_settings.height,
		.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.layer_count_or_depth = 1,
		.num_levels = 1
	};
	depth_stencil_texture = SDL_CreateGPUTexture(app.gpu_device, &depth_stencil_texture_info);
	if (!depth_stencil_texture) {
		LOG_ERROR("Failed to create depth stencil texture: %s", SDL_GetError());
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
			.max_anisotropy = 16,
			.min_lod = 0.0f,
			.max_lod = 1000.0f
		}
	);

	shadow_maps = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
			.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
			.width = SHADOW_MAP_RESOLUTION,
			.height = SHADOW_MAP_RESOLUTION,
			.layer_count_or_depth = MAX_LIGHTS,
			.num_levels = 1,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
		}
	);
}


void render_instances(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass,
			MeshData* mesh_data, Pipeline pipeline) {
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(gpu_command_buffer);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = mesh_data->instance_transfer_buffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data->instance_buffer,
			.offset = 0,
			.size = mesh_data->instance_size * mesh_data->num_instances
		},
		true
	);

	SDL_EndGPUCopyPass(copy_pass);

	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[pipeline]);
	SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding) { .buffer = mesh_data->vertex_buffer, .offset = 0 }, 1);
	SDL_BindGPUIndexBuffer(
		render_pass, &(SDL_GPUBufferBinding) { .buffer = mesh_data->index_buffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT
	);
	SDL_BindGPUVertexStorageBuffers(render_pass, 0, &mesh_data->instance_buffer, 1);

	if (pipeline == PIPELINE_3D_TEXTURED) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = resources.texture_array,
				.sampler = sampler,
			},
			1
		);
		SDL_BindGPUFragmentSamplers(
			render_pass,
			1,
			&(SDL_GPUTextureSamplerBinding){
				.texture = shadow_maps,
				.sampler = sampler,
			},
			1
		);
	}

	SDL_DrawGPUIndexedPrimitives(render_pass, mesh_data->num_indices, mesh_data->num_instances, 0, 0, 0);
}


void add_light(Vector3 position, Color diffuse_color, Color specular_color, Matrix4 projection_view) {
	LightData light_data = {
		.position = position,
		.diffuse_color = { diffuse_color.r / 255.0f, diffuse_color.g / 255.0f, diffuse_color.b / 255.0f },
		.specular_color = { specular_color.r / 255.0f, specular_color.g / 255.0f, specular_color.b / 255.0f },
		.projection_view_matrix = transpose4(projection_view),
	};

	memcpy(lights + num_lights, &light_data, sizeof(LightData));
	num_lights++;
}


void render_shadow_maps(SDL_GPUCommandBuffer* command_buffer) {
	for (Entity i = 0; i < scene->components->entities; i++) {
		LightComponent* light = get_component(i, COMPONENT_LIGHT);
		if (!light) continue;

		Matrix4 projection_view = transpose4(light->shadow_map.projection_view_matrix);
		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_view, sizeof(Matrix4));

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

		for (int j = 0; j < resources.meshes_size; j++) {
			render_instances(command_buffer, render_pass, &resources.meshes[j], PIPELINE_3D_TEXTURED);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	int layer = 0;
	for (Entity i = 0; i < scene->components->entities; i++) {
		LightComponent* light = get_component(i, COMPONENT_LIGHT);
		if (!light) continue;

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


void render() {
	command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
	if (!command_buffer) {
		LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
		return;
	}

	SDL_GPUTexture* swapchain_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window, &swapchain_texture, NULL, NULL);

	if (swapchain_texture) {
		render_shadow_maps(command_buffer);

		CameraComponent* camera = get_component(scene->camera, COMPONENT_CAMERA);
		Matrix4 view_matrix = transform_inverse(get_transform(scene->camera));
		// Need to shift so rotation happens around the center of the camera
		// Otherwise the camera "orbits"
		view_matrix._34 += 1.0f;
		Matrix4 projection_matrix = camera->projection_matrix;
		Matrix4 projection_view_matrix = transpose4(matrix4_mult(projection_matrix, view_matrix));

		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_view_matrix, sizeof(Matrix4));

		SDL_GPUColorTargetInfo color_target_info = {
			.texture = swapchain_texture,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }
		};
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

		UniformData uniform_data = {
			.near_plane = camera->near_plane,
			.far_plane = camera->far_plane,
			.ambient_light = scene->ambient_light,
			.num_lights = num_lights,
			.camera_position = get_position(scene->camera),
			.shadow_map_resolution = SHADOW_MAP_RESOLUTION,
		};
		SDL_PushGPUFragmentUniformData(command_buffer, 0, &uniform_data, sizeof(UniformData));
		SDL_PushGPUFragmentUniformData(command_buffer, 1, &lights, sizeof(LightData) * num_lights);

		for (int i = 0; i < resources.meshes_size; i++) {
			render_instances(command_buffer, render_pass, &resources.meshes[i], PIPELINE_3D_TEXTURED);
		}
		render_instances(command_buffer, render_pass, &triangle_mesh, PIPELINE_3D);

		SDL_EndGPURenderPass(render_pass);
	}

	// Reset instance counts for next frame
	num_lights = 0;
	for (int i = 0; i < resources.meshes_size; i++) {
		resources.meshes[i].num_instances = 0;
	}
	triangle_mesh.num_instances = 0;

	SDL_SubmitGPUCommandBuffer(command_buffer);
	command_buffer = NULL;
}


SDL_GPUBuffer* double_buffer_size(SDL_GPUBuffer* buffer, int size) {
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

	return new_buffer;
}


SDL_GPUTransferBuffer* double_transfer_buffer_size(SDL_GPUTransferBuffer* transfer_buffer, int size) {
	SDL_GPUTransferBuffer* new_transfer_buffer = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 2 * size,
			}
		);

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	void* data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);
	void* new_data = SDL_MapGPUTransferBuffer(app.gpu_device, new_transfer_buffer, false);
	SDL_memcpy(new_data, data, size);
	SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);
	SDL_UnmapGPUTransferBuffer(app.gpu_device, new_transfer_buffer);

	SDL_EndGPUCopyPass(copy_pass);

	SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return new_transfer_buffer;
}


void render_mesh(Matrix4 transform, int mesh_index, int texture_index, int material_index) {
	MeshData* mesh_data = &resources.meshes[mesh_index];

	if (mesh_data->num_instances >= mesh_data->max_instances) {
		LOG_INFO("Buffer %s full, resizing...", mesh_data->name);
		mesh_data->instance_buffer = double_buffer_size(
			mesh_data->instance_buffer,
			sizeof(InstanceData) * mesh_data->max_instances
		);
		mesh_data->instance_transfer_buffer = double_transfer_buffer_size(
			mesh_data->instance_transfer_buffer,
			sizeof(InstanceData) * mesh_data->max_instances
		);
		mesh_data->max_instances *= 2;
		LOG_INFO("New buffer size: %d", mesh_data->max_instances);
	}

	// TODO: Map the transfer buffer only once per frame
	InstanceData* transforms = SDL_MapGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer, false);

	InstanceData instance_data = {
		.transform = transpose4(transform),
		.material = resources.materials[material_index],
		.texture_index = texture_index,
	};
	transforms[mesh_data->num_instances] = instance_data;
	mesh_data->num_instances++;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer);
}


void render_triangle(Vector3 a, Vector3 b, Vector3 c, Color color) {
	Vector3 n = cross(
		diff3(b, a),
		diff3(c, a)
	);
	Matrix4 transform = {
		b.x - a.x, c.x - a.x, n.x, a.x,
		b.y - a.y, c.y - a.y, n.y, a.y,
		b.z - a.z, c.z - a.z, n.z, a.z,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	if (triangle_mesh.num_instances >= triangle_mesh.max_instances) {
		LOG_INFO("Buffer full, resizing...");
		triangle_mesh.instance_buffer = double_buffer_size(
			triangle_mesh.instance_buffer,
			sizeof(InstanceColorData) * triangle_mesh.max_instances
		);
		triangle_mesh.instance_transfer_buffer = double_transfer_buffer_size(
			triangle_mesh.instance_transfer_buffer,
			sizeof(InstanceColorData) * triangle_mesh.max_instances
		);
		triangle_mesh.max_instances *= 2;
		LOG_INFO("New buffer size: %d", triangle_mesh.max_instances);
	}

	InstanceColorData* instance_datas = SDL_MapGPUTransferBuffer(app.gpu_device, triangle_mesh.instance_transfer_buffer, false);
	InstanceColorData instance_data = {
		.transform = transpose4(transform),
		.color = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f }
	};
	instance_datas[triangle_mesh.num_instances] = instance_data;
	triangle_mesh.num_instances++;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, triangle_mesh.instance_transfer_buffer);
}


void render_line(Vector3 start, Vector3 end, float thickness, Color color) {
	Vector3 direction = diff3(end, start);
	if (norm3(direction) < 1e-6f) return; // Avoid zero-length lines

	Vector3 up = {0.0f, 0.0f, 1.0f};
	if (fabsf(dot3(normalized3(direction), up)) > 0.99f) {
		up = (Vector3){0.0f, 1.0f, 0.0f}; // Use a different up if parallel
	}
	Vector3 perpendicular = normalized3(cross(direction, up));
	Vector3 half_offset = mult3(thickness / 2.0f, perpendicular);

	Vector3 a = sum3(start, half_offset);
	Vector3 b = sum3(end, half_offset);
	Vector3 c = diff3(end, half_offset);
	Vector3 d = diff3(start, half_offset);

	render_triangle(a, b, c, color);
	render_triangle(a, c, d, color);
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
	render_triangle(d, a, b, color);
}


void render_arrow(Vector3 start, Vector3 end, float thickness, Color color) {
	// Arrow tip size
	float tip_length = 4.0f * thickness;
	float tip_width = 5.0f * thickness;

	Vector3 direction = diff3(end, start);
	float len = norm3(direction);
	if (len < 1e-6f) return;
	Vector3 dir = normalized3(direction);

	render_line(
		start,
		sum3(start, mult3(fmaxf(len - tip_length, 0.0f), dir)),
		thickness,
		color
	);

	Vector3 up = {0.0f, 0.0f, 1.0f};
	if (fabsf(dot3(dir, up)) > 0.99f) {
		up = (Vector3){0.0f, 1.0f, 0.0f};
	}
	Vector3 perp = normalized3(cross(dir, up));

	Vector3 tip_base = diff3(end, mult3(tip_length, dir));
	Vector3 left = sum3(tip_base, mult3(tip_width / 2.0f, perp));
	Vector3 right = diff3(tip_base, mult3(tip_width / 2.0f, perp));

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
	Vector3 center = mult3(plane.offset, plane.normal);
	Vector3 a = sum3(center, mult3(size, right));
	Vector3 b = sum3(center, mult3(size, forward));
	Vector3 c = diff3(center, mult3(size, right));
	Vector3 d = diff3(center, mult3(size, forward));

	render_quad(a, b, c, d, color);
}
