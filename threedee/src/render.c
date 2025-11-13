#define _USE_MATH_DEFINES

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"
#include "component.h"
#include "resources.h"
#include "scene.h"
#include "settings.h"
#include "app.h"
#include "util.h"


typedef enum Shader {
	SHADER_VERTEX_POSITION_COLOR_2D,
	SHADER_VERTEX_POSITION_COLOR,
	SHADER_VERTEX_POSITION_TEXTURE,
	SHADER_VERTEX_TEXT,
	SHADER_VERTEX_SHADOW_DEPTH,
	SHADER_VERTEX_POST_PROCESSING,
	SHADER_VERTEX_BILLBOARD,
	SHADER_VERTEX_LINE,
	SHADER_FRAGMENT_SOLID_COLOR,
	SHADER_FRAGMENT_SOLID_COLOR_DEPTH,
	SHADER_FRAGMENT_PHONG,
	SHADER_FRAGMENT_SHADOW_DEPTH,
	SHADER_FRAGMENT_TEXT,
	SHADER_FRAGMENT_POST_PROCESSING,
	SHADER_FRAGMENT_DEPTH_OF_FIELD,
	SHADER_COUNT
} Shader;


typedef enum {
	PIPELINE_2D,
	PIPELINE_TEXT,
	PIPELINE_3D,
	PIPELINE_3D_TEXTURED,
	PIPELINE_SHADOW_DEPTH,
	PIPELINE_POST_PROCESSING,
	PIPELINE_DEPTH_OF_FIELD,
	PIPELINE_BILLBOARD,
	PIPELINE_LINE,
	PIPELINE_COUNT
} Pipeline;


static int frame_index = 0;
SDL_GPUFence* fences[FRAMES_IN_FLIGHT] = { 0 };

static SDL_GPUShader* shaders[SHADER_COUNT] = { 0 };
static SDL_GPUGraphicsPipeline* pipelines[PIPELINE_COUNT] = { 0 };
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


static const SDL_GPUVertexInputState VERTEX_INPUT_STATE_POSITION_TEXTURE_VERTEX = {
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
};


static const SDL_GPUColorTargetBlendState BLEND_STATE = {
	.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	.color_blend_op = SDL_GPU_BLENDOP_ADD,
	.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
	.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
	.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	.enable_blend = true
};


SDL_GPUSampleCount get_sample_count() {
	switch (game_settings.antialiasing) {
		case 0: return SDL_GPU_SAMPLECOUNT_1;
		case 2: return SDL_GPU_SAMPLECOUNT_2;
		case 4: return SDL_GPU_SAMPLECOUNT_4;
		case 8: return SDL_GPU_SAMPLECOUNT_8;
		default: return SDL_GPU_SAMPLECOUNT_1;
	}
}


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


void load_shaders() {
	shaders[SHADER_VERTEX_POSITION_COLOR_2D] = load_shader(app.gpu_device, "position_color_2d.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_POSITION_COLOR] = load_shader(app.gpu_device, "position_color.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_POSITION_TEXTURE] = load_shader(app.gpu_device, "position_texture.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_TEXT] = load_shader(app.gpu_device, "text.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_SHADOW_DEPTH] = load_shader(app.gpu_device, "shadow_depth.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_POST_PROCESSING] = load_shader(app.gpu_device, "post_processing.vert", 0, 0, 0, 0);
	shaders[SHADER_VERTEX_BILLBOARD] = load_shader(app.gpu_device, "billboard.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_LINE] = load_shader(app.gpu_device, "line.vert", 0, 1, 1, 0);
	shaders[SHADER_FRAGMENT_SOLID_COLOR] = load_shader(app.gpu_device, "solid_color.frag", 0, 0, 0, 0);
	shaders[SHADER_FRAGMENT_SOLID_COLOR_DEPTH] = load_shader(app.gpu_device, "solid_color_depth.frag", 0, 1, 0, 0);
	shaders[SHADER_FRAGMENT_PHONG] = load_shader(app.gpu_device, "phong.frag", 4, 2, 1, 0);
	shaders[SHADER_FRAGMENT_SHADOW_DEPTH] = load_shader(app.gpu_device, "shadow_depth.frag", 0, 0, 0, 0);
	shaders[SHADER_FRAGMENT_TEXT] = load_shader(app.gpu_device, "text.frag", 1, 0, 0, 0);
	shaders[SHADER_FRAGMENT_POST_PROCESSING] = load_shader(app.gpu_device, "post_processing.frag", 2, 1, 0, 0);
	shaders[SHADER_FRAGMENT_DEPTH_OF_FIELD] = load_shader(app.gpu_device, "depth_of_field.frag", 2, 1, 0, 0);
}


SDL_GPUGraphicsPipeline* create_render_pipeline_2d() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(Vector2)
			}},
			.num_vertex_attributes = 1,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 0,
				.offset = 0
			}}
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = shaders[SHADER_VERTEX_POSITION_COLOR_2D],
		.fragment_shader = shaders[SHADER_FRAGMENT_SOLID_COLOR],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_text() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(PositionTextureVertex2D)
			}},
			.num_vertex_attributes = 2,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 0,
				.offset = 0
			}, {
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.location = 1,
				.offset = sizeof(float) * 2
			}}
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = shaders[SHADER_VERTEX_TEXT],
		.fragment_shader = shaders[SHADER_FRAGMENT_TEXT],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_3d() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = DEPTH_FORMAT
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
				.slot = 0,
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
				.pitch = sizeof(Vector3)
			}},
			.num_vertex_attributes = 1,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){{
				.buffer_slot = 0,
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.location = 0,
				.offset = 0
			}}
		},
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.multisample_state = (SDL_GPUMultisampleState) {
			.sample_count = get_sample_count()
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = shaders[SHADER_VERTEX_POSITION_COLOR],
		.fragment_shader = shaders[SHADER_FRAGMENT_SOLID_COLOR_DEPTH],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_3d_textured() {
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING, "3d textured");

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = DEPTH_FORMAT
		},
		.vertex_input_state = VERTEX_INPUT_STATE_POSITION_TEXTURE_VERTEX,
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.multisample_state = (SDL_GPUMultisampleState) {
			.sample_count = get_sample_count()
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = shaders[SHADER_VERTEX_POSITION_TEXTURE],
		.fragment_shader = shaders[SHADER_FRAGMENT_PHONG],
		.props = props
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_DestroyProperties(props);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_shadow_depth() {
	bool valid = SDL_GPUTextureSupportsFormat(
		app.gpu_device,
		DEPTH_FORMAT,
		SDL_GPU_TEXTURETYPE_2D,
		SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
	);
	LOG_INFO("Depth format supported for depth-stencil target: %s", valid ? "yes" : "no");

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 0,
			.color_target_descriptions = NULL,
			.has_depth_stencil_target = true,
			.depth_stencil_format = DEPTH_FORMAT
		},
		.vertex_input_state = VERTEX_INPUT_STATE_POSITION_TEXTURE_VERTEX,
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
		.vertex_shader = shaders[SHADER_VERTEX_SHADOW_DEPTH],
		.fragment_shader = shaders[SHADER_FRAGMENT_SHADOW_DEPTH],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_post_processing() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window)
			}},
			.has_depth_stencil_target = false
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_shader = shaders[SHADER_VERTEX_POST_PROCESSING],
		.fragment_shader = shaders[SHADER_FRAGMENT_POST_PROCESSING],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_depth_of_field() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window)
			}},
			.has_depth_stencil_target = false
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_shader = shaders[SHADER_VERTEX_POST_PROCESSING],
		.fragment_shader = shaders[SHADER_FRAGMENT_DEPTH_OF_FIELD],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_billboard() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = DEPTH_FORMAT
		},
		.vertex_input_state = VERTEX_INPUT_STATE_POSITION_TEXTURE_VERTEX,
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.multisample_state = (SDL_GPUMultisampleState) {
			.sample_count = get_sample_count()
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = false,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = shaders[SHADER_VERTEX_BILLBOARD],
		.fragment_shader = shaders[SHADER_FRAGMENT_PHONG],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_line() {
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(app.gpu_device, app.window),
				.blend_state = BLEND_STATE
			}},
			.has_depth_stencil_target = true,
			.depth_stencil_format = DEPTH_FORMAT
		},
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_NONE,
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
		},
		.multisample_state = (SDL_GPUMultisampleState) {
			.sample_count = get_sample_count()
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = false,
			.compare_op = SDL_GPU_COMPAREOP_LESS
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP,
		.vertex_shader = shaders[SHADER_VERTEX_LINE],
		.fragment_shader = shaders[SHADER_FRAGMENT_SOLID_COLOR_DEPTH],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

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
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh_data.num_vertices = 3;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector3) * mesh_data.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector3) * mesh_data.num_vertices,
        }
    );

    mesh_data.instance_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(InstanceColorData) * mesh_data.max_instances * FRAMES_IN_FLIGHT,
        }
    );

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		mesh_data.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = sizeof(InstanceColorData) * mesh_data.max_instances,
			}
		);
	}

    Vector3* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vector3) { 0.0f, 0.0f, 0.0f };
	transfer_data[1] = (Vector3) { 1.0f, 0.0f, 0.0f };
	transfer_data[2] = (Vector3) { 0.0f, 1.0f, 0.0f };

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

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}


MeshData create_mesh_triangle_2d() {
	MeshData mesh_data = {
		.name = "triangle_2d",
		.max_instances = 256,
		.num_instances = 0,
		.instance_size = sizeof(InstanceColorData2D),
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh_data.num_vertices = 3;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector2) * mesh_data.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector2) * mesh_data.num_vertices,
        }
    );

    mesh_data.instance_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(InstanceColorData2D) * mesh_data.max_instances * FRAMES_IN_FLIGHT,
        }
    );

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		mesh_data.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = sizeof(InstanceColorData2D) * mesh_data.max_instances,
			}
		);
	}

    Vector2* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vector2) { 0.0f, 0.0f };
	transfer_data[1] = (Vector2) { 1.0f, 0.0f };
	transfer_data[2] = (Vector2) { 0.0f, 1.0f };

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
            .size = sizeof(Vector2) * mesh_data.num_vertices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}


MeshData create_mesh_quad() {
	MeshData mesh_data = {
		.name = "quad",
		.num_vertices = 4,
		.num_indices = 6,
		.num_instances = 0,
		.max_instances = 256,
		.instance_size = sizeof(BillboardInstanceData),
	};

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
			.size = mesh_data.instance_size * mesh_data.max_instances * FRAMES_IN_FLIGHT,
		}
	);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		mesh_data.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = mesh_data.instance_size * mesh_data.max_instances,
			}
		);
	}

	SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
		}
	);

	PositionTextureVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);
	transfer_data[0] = (PositionTextureVertex) {
		.position = { -0.5f, -0.5f, 0.0f },
		.uv = { 0.0f, 1.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[1] = (PositionTextureVertex) {
		.position = { 0.5f, -0.5f, 0.0f },
		.uv = { 1.0f, 1.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[2] = (PositionTextureVertex) {
		.position = { 0.5f, 0.5f, 0.0f },
		.uv = { 1.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[3] = (PositionTextureVertex) {
		.position = { -0.5f, 0.5f, 0.0f },
		.uv = { 0.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};

	Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
	index_data[0] = 0;
	index_data[1] = 1;
	index_data[2] = 2;
	index_data[3] = 2;
	index_data[4] = 3;
	index_data[5] = 0;

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


MeshData create_mesh_line() {
	MeshData mesh_data = {
		.name = "line",
		.num_vertices = 4,
		.num_indices = 0,
		.num_instances = 0,
		.max_instances = 256,
		.instance_size = sizeof(LineInstanceData),
	};

	mesh_data.instance_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
			.size = mesh_data.instance_size * mesh_data.max_instances * FRAMES_IN_FLIGHT,
		}
	);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		mesh_data.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = mesh_data.instance_size * mesh_data.max_instances,
			}
		);
	}

	return mesh_data;
}


Vector2 get_text_center(TTF_GPUAtlasDrawSequence data) {
	float min_x = INFINITY;
	float min_y = INFINITY;
	float max_x = -INFINITY;
	float max_y = -INFINITY;

	for (int i = 0; i < data.num_vertices; ++i) {
		SDL_FPoint xy = data.xy[i];
		min_x = fminf(min_x, xy.x);
		min_y = fminf(min_y, xy.y);
		max_x = fmaxf(max_x, xy.x);
		max_y = fmaxf(max_y, xy.y);
	}

	return (Vector2) {
		.x = (min_x + max_x) * 0.5f,
		.y = (min_y + max_y) * 0.5f
	};
}


MeshData create_mesh_text(TTF_GPUAtlasDrawSequence data) {
	MeshData mesh_data = {
		.name = "text",
		.num_vertices = data.num_vertices,
		.num_indices = data.num_indices,
		.texture = data.atlas_texture,
		.num_instances = 0,
		.max_instances = 256,
		.instance_size = sizeof(InstanceColorData2D),
	};

    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices,
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
			.size = sizeof(InstanceColorData2D) * mesh_data.max_instances * FRAMES_IN_FLIGHT,
		}
	);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		mesh_data.instance_transfer_buffer[i] = SDL_CreateGPUTransferBuffer(
			app.gpu_device,
			&(SDL_GPUTransferBufferCreateInfo){
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = sizeof(InstanceColorData2D) * mesh_data.max_instances,
			}
		);
	}

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        	.size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
        }
    );

    PositionTextureVertex2D* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

	Vector2 text_center = get_text_center(data);
	float w = text_center.x;
	float h = text_center.y;

    for (int i = 0; i < mesh_data.num_vertices; ++i) {
    	SDL_FPoint xy = data.xy[i];
    	SDL_FPoint uv = data.uv[i];
		transfer_data[i] = (PositionTextureVertex2D) { xy.x - w, xy.y - h, uv.x, uv.y };
	}

	Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
	for (int i = 0; i < mesh_data.num_indices; ++i) {
		index_data[i] = data.indices[i];
	}

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
            .size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices
        },
        false
    );

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices
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


void init_render() {
	app.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

	SDL_SetGPUSwapchainParameters(
		app.gpu_device,
		app.window,
		SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
		game_settings.vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE
	);

	load_shaders();

	pipelines[PIPELINE_2D] = create_render_pipeline_2d();
	pipelines[PIPELINE_TEXT] = create_render_pipeline_text();
	pipelines[PIPELINE_3D] = create_render_pipeline_3d();
	pipelines[PIPELINE_3D_TEXTURED] = create_render_pipeline_3d_textured();
	pipelines[PIPELINE_SHADOW_DEPTH] = create_render_pipeline_shadow_depth();
	pipelines[PIPELINE_POST_PROCESSING] = create_render_pipeline_post_processing();
	pipelines[PIPELINE_DEPTH_OF_FIELD] = create_render_pipeline_depth_of_field();
	pipelines[PIPELINE_BILLBOARD] = create_render_pipeline_billboard();
	pipelines[PIPELINE_LINE] = create_render_pipeline_line();

	triangle_mesh = create_mesh_triangle();
	triangle_2d_mesh = create_mesh_triangle_2d();
	quad_mesh = create_mesh_quad();
	line_mesh = create_mesh_line();
	texts = ArrayList_create(sizeof(MeshData));

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
	SDL_ReleaseGPUBuffer(app.gpu_device, mesh_data->instance_buffer);
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		SDL_ReleaseGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer[i]);
	}

	if (mesh_data->index_buffer) {
		SDL_ReleaseGPUBuffer(app.gpu_device, mesh_data->index_buffer);
	}
}


void apply_render_settings() {
	// Needs to be called if resolution, antialiasing settings change
	SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipelines[PIPELINE_3D]);
	SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipelines[PIPELINE_3D_TEXTURED]);
	pipelines[PIPELINE_3D] = create_render_pipeline_3d();
	pipelines[PIPELINE_3D_TEXTURED] = create_render_pipeline_3d_textured();

	SDL_ReleaseGPUTexture(app.gpu_device, depth_stencil_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, screen_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, resolve_texture);
	SDL_ReleaseGPUTexture(app.gpu_device, dof_temp_texture);
	create_screen_textures();
}


void render_mesh(SDL_GPURenderPass* render_pass, MeshData* mesh_data, Pipeline pipeline) {
	SDL_BindGPUGraphicsPipeline(render_pass, pipelines[pipeline]);
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
				.texture = resources.normal_map_array,
				.sampler = sampler,
			},
			1
		);
		SDL_BindGPUFragmentSamplers(
			render_pass,
			2,
			&(SDL_GPUTextureSamplerBinding){
				.texture = shadow_maps,
				.sampler = sampler,
			},
			1
		);
		SDL_BindGPUFragmentSamplers(
			render_pass,
			3,
			&(SDL_GPUTextureSamplerBinding){
				.texture = resources.emissive_map_array,
				.sampler = sampler,
			},
			1
		);
	}

	if (mesh_data->index_buffer) {
		SDL_BindGPUIndexBuffer(
			render_pass, &(SDL_GPUBufferBinding) { .buffer = mesh_data->index_buffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT
		);

		SDL_DrawGPUIndexedPrimitives(
			render_pass,
			mesh_data->num_indices,
			mesh_data->num_instances,
			0,
			0,
			frame_index * mesh_data->max_instances
		);
	} else {
		SDL_DrawGPUPrimitives(
			render_pass,
			mesh_data->num_vertices,
			mesh_data->num_instances,
			0,
			frame_index * mesh_data->max_instances
		);
	}
}


void render_instances(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass,
			MeshData* mesh_data, Pipeline pipeline) {
	if (mesh_data->num_instances == 0) {
		return;
	}

	if (mesh_data->instance_size == 0) {
		LOG_ERROR("Instance size is zero");
	}

	if (mesh_data->instance_data[frame_index]) {
		LOG_DEBUG("Mesh %s has instance data still mapped, unmapping now", mesh_data->name);
		SDL_UnmapGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer[frame_index]);
		mesh_data->instance_data[frame_index] = NULL;
	}

	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(gpu_command_buffer);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = mesh_data->instance_transfer_buffer[frame_index],
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data->instance_buffer,
			.offset = frame_index * mesh_data->max_instances * mesh_data->instance_size,
			.size = mesh_data->instance_size * mesh_data->num_instances
		},
		true
	);

	SDL_EndGPUCopyPass(copy_pass);

	SDL_BindGPUVertexStorageBuffers(render_pass, 0, &mesh_data->instance_buffer, 1);

	render_mesh(render_pass, mesh_data, pipeline);
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

		for (int j = 0; j < resources.meshes_size; j++) {
			render_instances(command_buffer, render_pass, &resources.meshes[j], PIPELINE_SHADOW_DEPTH);
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
		&(SDL_GPUTextureSamplerBinding){
			.texture = source,
			.sampler = screen_sampler,
		},
		1
	);
	SDL_BindGPUFragmentSamplers(
		render_pass,
		1,
		&(SDL_GPUTextureSamplerBinding){
			.texture = depth_stencil_texture,
			.sampler = screen_sampler,
		},
		1
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

		for (int i = 0; i < resources.meshes_size; i++) {
			render_instances(command_buffer, render_pass, &resources.meshes[i], PIPELINE_3D_TEXTURED);
		}
		render_instances(command_buffer, render_pass, &triangle_mesh, PIPELINE_3D);
		render_instances(command_buffer, render_pass, &quad_mesh, PIPELINE_BILLBOARD);
		render_instances(command_buffer, render_pass, &line_mesh, PIPELINE_LINE);

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
			&(SDL_GPUTextureSamplerBinding){
				.texture = source_texture,
				.sampler = screen_sampler,
			},
			1
		);
		SDL_BindGPUFragmentSamplers(
			render_pass,
			1,
			&(SDL_GPUTextureSamplerBinding){
				.texture = depth_stencil_texture,
				.sampler = screen_sampler,
			},
			1
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

		render_instances(command_buffer, render_pass, &triangle_2d_mesh, PIPELINE_2D);

		for (int i = 0; i < texts->size; i++) {
			MeshData* text = ArrayList_get(texts, i);
			render_instances(command_buffer, render_pass, text, PIPELINE_TEXT);
		}

		SDL_EndGPURenderPass(render_pass);
	}

	fences[frame_index] = SDL_SubmitGPUCommandBufferAndAcquireFence(command_buffer);

	LOG_DEBUG("Submitted frame %d", frame_index);

	// Reset instance counts for next frame
	num_lights = 0;
	for (int i = 0; i < resources.meshes_size; i++) {
		resources.meshes[i].num_instances = 0;
	}
	triangle_mesh.num_instances = 0;
	triangle_2d_mesh.num_instances = 0;
	quad_mesh.num_instances = 0;
	line_mesh.num_instances = 0;
	ArrayList_for_each(texts, destroy_mesh);
	ArrayList_clear(texts);

	frame_index = (frame_index + 1) % FRAMES_IN_FLIGHT;
}


void* get_instance_data(MeshData* mesh_data) {
	if (mesh_data->instance_data[frame_index]) {
		return mesh_data->instance_data[frame_index];
	}

	LOG_DEBUG("Mapping instance data for mesh %s, frame %d", mesh_data->name, frame_index);
	mesh_data->instance_data[frame_index] = SDL_MapGPUTransferBuffer(
		app.gpu_device, mesh_data->instance_transfer_buffer[frame_index], false
	);
	return mesh_data->instance_data[frame_index];
}


SDL_GPUBuffer* double_buffer_size(SDL_GPUCommandBuffer* command_buffer, SDL_GPUBuffer* buffer, int size) {
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


void double_instance_buffer_sizes(MeshData* mesh_data) {
	SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);

	int size = mesh_data->instance_size * mesh_data->max_instances;

	// Instance buffer
	mesh_data->instance_buffer = double_buffer_size(
		command_buffer,
		mesh_data->instance_buffer,
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
			LOG_DEBUG("Copying instance transfer buffer data for mesh %s, frame %d", mesh_data->name, i);
			SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);

			void* data = get_instance_data(mesh_data);
			mesh_data->instance_data[i] = SDL_MapGPUTransferBuffer(app.gpu_device, new_transfer_buffer, false);

			SDL_memcpy(mesh_data->instance_data[i], data, size);

			// Unmap old transfer buffer, keep new one mapped
			SDL_UnmapGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer[i]);

			SDL_EndGPUCopyPass(copy_pass);
		}

		SDL_ReleaseGPUTransferBuffer(app.gpu_device, mesh_data->instance_transfer_buffer[i]);
		mesh_data->instance_transfer_buffer[i] = new_transfer_buffer;
	}

	mesh_data->max_instances *= 2;
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


void draw_mesh(
	Matrix4 transform,
	int mesh_index,
	int texture_index,
	int material_index,
	int emissive_index,
	Visibility visibility,
	Vector2 texture_scale
) {
	LOG_DEBUG("Drawing mesh %d with texture %d", mesh_index, texture_index);

	MeshData* mesh_data = &resources.meshes[mesh_index];

	if (mesh_data->num_instances >= mesh_data->max_instances) {
		wait_for_fences();

		LOG_INFO("Buffer %s full, resizing...", mesh_data->name);
		double_instance_buffer_sizes(mesh_data);
		LOG_INFO("New buffer size: %d", mesh_data->max_instances);
	}

	InstanceData* transforms = get_instance_data(mesh_data);

	InstanceData instance_data = {
		.transform = transpose4(transform),
		.material_index = material_index,
		.texture_index = texture_index,
		.emissive_index = emissive_index,
		.texture_scale = texture_scale,
		.visiblity = visibility,
	};
	transforms[mesh_data->num_instances] = instance_data;
	mesh_data->num_instances++;
}


void draw_sprite(Vector3 position, float width, float height, int texture_index) {
	LOG_DEBUG("Drawing mesh %d with texture %d", mesh_index, texture_index);

	if (quad_mesh.num_instances >= quad_mesh.max_instances) {
		wait_for_fences();

		LOG_INFO("Buffer %s full, resizing...", quad_mesh.name);
		double_instance_buffer_sizes(&quad_mesh);
		LOG_INFO("New buffer size: %d", quad_mesh.max_instances);
	}

	BillboardInstanceData* instances = get_instance_data(&quad_mesh);

	BillboardInstanceData instance_data = {
		.position = position,
		.width = width,
		.height = height,
		.texture_index = texture_index,
		.material = resources.materials[1],
		.visiblity = VISIBILITY_ALL,
		.type = BILLBOARD_CYLINDRICAL
	};
	instances[quad_mesh.num_instances] = instance_data;
	quad_mesh.num_instances++;
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

	if (triangle_mesh.num_instances >= triangle_mesh.max_instances) {
		wait_for_fences();

		LOG_INFO("Buffer full, resizing...");
		double_instance_buffer_sizes(&triangle_mesh);
		LOG_INFO("New buffer size: %d", triangle_mesh.max_instances);
	}

	InstanceColorData* instance_datas = get_instance_data(&triangle_mesh);
	InstanceColorData instance_data = {
		.transform = transpose4(transform),
		.color = color
	};
	instance_datas[triangle_mesh.num_instances] = instance_data;
	triangle_mesh.num_instances++;
}


void draw_line(Vector3 start, Vector3 end, float thickness, Color color) {
	LOG_DEBUG("Drawing line from (%f, %f, %f) to (%f, %f, %f)", start.x, start.y, start.z, end.x, end.y, end.z);

	if (line_mesh.num_instances >= line_mesh.max_instances) {
		wait_for_fences();

		LOG_INFO("Buffer %s full, resizing...", line_mesh.name);
		double_instance_buffer_sizes(&line_mesh);
		LOG_INFO("New buffer size: %d", line_mesh.max_instances);
	}

	LineInstanceData* instances = get_instance_data(&line_mesh);

	LineInstanceData instance_data = {
		.start = start,
		.end = end,
		.thickness = thickness,
		.color = color
	};
	instances[line_mesh.num_instances] = instance_data;
	line_mesh.num_instances++;
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
	LOG_DEBUG("Drawing 2D triangle at (%.2f, %.2f), (%.2f, %.2f), (%.2f, %.2f)", a.x, a.y, b.x, b.y, c.x, c.y);

	if (triangle_2d_mesh.num_instances >= triangle_2d_mesh.max_instances) {
		wait_for_fences();

		LOG_INFO("Buffer full, resizing...");
		double_instance_buffer_sizes(&triangle_2d_mesh);
		LOG_INFO("New buffer size: %d", triangle_2d_mesh.max_instances);
	}

	InstanceColorData2D* instance_datas = get_instance_data(&triangle_2d_mesh);
	InstanceColorData2D instance_data = {
		.transform = {
			b.x - a.x, c.x - a.x, a.x, 0.0f,
			b.y - a.y, c.y - a.y, a.y, 0.0f,
		},
		.color = color
	};
	instance_datas[triangle_2d_mesh.num_instances] = instance_data;
	triangle_2d_mesh.num_instances++;

	LOG_DEBUG("2D triangle drawn");
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
	strcpy(mesh_data.name, text->text);

	// Match text pixel size to screen coordinates
	float scale = size / 216.0f;

	InstanceColorData2D* instance_datas = SDL_MapGPUTransferBuffer(
		app.gpu_device, mesh_data.instance_transfer_buffer[frame_index], false
	);
	InstanceColorData2D instance_data = {
		.transform = {
			scale * cosf(angle), -scale * sinf(angle), position.x, 0.0f,
			scale * sinf(angle), scale * cosf(angle), position.y, 0.0f
		},
		.color = color
	};
	instance_datas[mesh_data.num_instances] = instance_data;
	mesh_data.num_instances++;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, mesh_data.instance_transfer_buffer[frame_index]);

	ArrayList_add(texts, &mesh_data);
}
