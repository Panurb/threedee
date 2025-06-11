#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"

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

static LightData lights[32];
static int num_lights = 0;

static MeshData meshes[3];


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


MeshData create_mesh_quad() {
	MeshData render_mode = {
		.max_instances = 256,
		.num_instances = 0
	};

	render_mode.num_vertices = 4;
    render_mode.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionColorVertex) * render_mode.num_vertices,
        }
    );

    render_mode.num_indices = 6;
    render_mode.index_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * render_mode.num_indices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(PositionColorVertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
        }
    );

    render_mode.instance_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(Matrix4) * render_mode.max_instances,
        }
    );

    render_mode.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Matrix4),
        }
    );

    PositionColorVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (PositionColorVertex) { {-0.5f, -0.5f, 0.0f}, {255, 0, 0, 255}};
    transfer_data[1] = (PositionColorVertex) { {0.5f, -0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[2] = (PositionColorVertex) { {0.5f, 0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[3] = (PositionColorVertex) { {-0.5f, 0.5f, 0.0f}, {0, 0, 255, 255} };

    Uint16* index_data = (Uint16*) &transfer_data[render_mode.num_vertices];
    const Uint16 indices[6] = { 0, 1, 2, 0, 2, 3 };
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
            .buffer = render_mode.vertex_buffer,
            .offset = 0,
            .size = sizeof(PositionColorVertex) * render_mode.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(PositionColorVertex) * render_mode.num_vertices
        },
        &(SDL_GPUBufferRegion) {
            .buffer = render_mode.index_buffer,
            .offset = 0,
            .size = sizeof(Uint16) * render_mode.num_indices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return render_mode;
}


MeshData create_mesh_cube() {
	MeshData mesh_data = {
		.max_instances = 256,
		.num_instances = 0
	};

	mesh_data.num_vertices = 8;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionColorVertex) * mesh_data.num_vertices,
        }
    );

    mesh_data.num_indices = 36;
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
            .size = sizeof(PositionColorVertex) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
        }
    );

    mesh_data.instance_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(Matrix4) * mesh_data.max_instances,
        }
    );

    mesh_data.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Matrix4),
        }
    );

    PositionColorVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (PositionColorVertex) { {-0.5f, -0.5f, -0.5f}, {255, 0, 0, 255}};
	transfer_data[1] = (PositionColorVertex) { {0.5f, -0.5f, -0.5f}, {255, 255, 0, 255}};
	transfer_data[2] = (PositionColorVertex) { {0.5f, 0.5f, -0.5f}, {0, 255, 0, 255}};
	transfer_data[3] = (PositionColorVertex) { {-0.5f, 0.5f, -0.5f}, {0, 255, 255, 255}};
	transfer_data[4] = (PositionColorVertex) { {-0.5f, -0.5f, 0.5f}, {255, 0, 255, 255}};
	transfer_data[5] = (PositionColorVertex) { {0.5f, -0.5f, 0.5f}, {255, 255, 255, 255}};
	transfer_data[6] = (PositionColorVertex) { {0.5f, 0.5f, 0.5f}, {128, 128, 128, 255}};
	transfer_data[7] = (PositionColorVertex) { {-0.5f, 0.5f, 0.5f}, {64, 64, 64, 255} };

	// Front-facing triangle should go clockwise
    Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
    const Uint16 indices[36] = {
    	// Back face (z = -0.5)
    	0, 2, 1, 0, 3, 2,
		// Front face (z = +0.5)
		4, 5, 6, 4, 6, 7,
		// Bottom face (y = -0.5)
		0, 1, 5, 0, 5, 4,
		// Top face (y = +0.5)
		3, 7, 6, 3, 6, 2,
		// Left face (x = -0.5)
		0, 4, 7, 0, 7, 3,
		// Right face (x = +0.5)
		1, 2, 6, 1, 6, 5
	};
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
            .size = sizeof(PositionColorVertex) * mesh_data.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(PositionColorVertex) * mesh_data.num_vertices
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

	meshes[MESH_QUAD] = create_mesh_quad();
	meshes[MESH_CUBE] = create_mesh_cube();

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
			.min_filter = SDL_GPU_FILTER_NEAREST,
			.mag_filter = SDL_GPU_FILTER_NEAREST,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
			.enable_anisotropy = true,
			.max_anisotropy = 16,
		}
	);

	shadow_maps = SDL_CreateGPUTexture(
		app.gpu_device,
		&(SDL_GPUTextureCreateInfo){
			.type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
			.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
			.width = 2048,
			.height = 2048,
			.layer_count_or_depth = 32, // Max number of lights
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
			.size = sizeof(InstanceData) * mesh_data->num_instances
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


void add_light(Vector3 position, Color diffuse_color, Color specular_color, Matrix4 view_projection) {
	LightData light_data = {
		.position = position,
		.diffuse_color = { diffuse_color.r / 255.0f, diffuse_color.g / 255.0f, diffuse_color.b / 255.0f },
		.specular_color = { specular_color.r / 255.0f, specular_color.g / 255.0f, specular_color.b / 255.0f },
		.view_projection_matrix = view_projection,
	};

	memcpy(lights + num_lights, &light_data, sizeof(LightData));
	num_lights++;
}


void render_shadow_maps(SDL_GPUCommandBuffer* command_buffer) {
	for (Entity i = 0; i < scene->components->entities; i++) {
		LightComponent* light = get_component(i, COMPONENT_LIGHT);
		if (!light) continue;

		Matrix4 view_matrix = transform_inverse(get_transform(i));
		// view_matrix._34 += 1.0f;
		Matrix4 projection_matrix = orthographic_projection_matrix(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
		Matrix4 view_projection = matrix4_mult(projection_matrix, view_matrix);
		light->shadow_map.view_projection_matrix = view_projection;

		SDL_PushGPUVertexUniformData(command_buffer, 0, &view_projection, sizeof(Matrix4));

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
			2048,
			2048,
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
		for (Entity entity = 0; entity < scene->components->entities; entity++) {
			LightComponent* light_component = get_component(entity, COMPONENT_LIGHT);
			if (light_component) {
				add_light(
					get_position(entity),
					light_component->diffuse_color,
					light_component->specular_color,
					light_component->shadow_map.view_projection_matrix
				);
			}

			MeshComponent* mesh_component = get_component(entity, COMPONENT_MESH);
			if (mesh_component) {
				add_render_instance(
					mesh_component->mesh_index,
					get_transform(entity),
					mesh_component->texture_index,
					mesh_component->material_index
				);
			}
		}

		render_shadow_maps(command_buffer);

		CameraComponent* camera = CameraComponent_get(scene->camera);
		Matrix4 view_matrix = transform_inverse(get_transform(scene->camera));
		// Need to shift so rotation happens around the center of the camera
		// Otherwise the camera "orbits"
		view_matrix._34 += 1.0f;
		Matrix4 projection_matrix = camera->projection_matrix;
		Matrix4 projection_view_matrix = transpose4(matrix4_mult(projection_matrix, view_matrix));

		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_view_matrix, sizeof(Matrix4));
		// SDL_PushGPUFragmentUniformData(command_buffer, 0, (float[]) { camera->near_plane, camera->far_plane }, 8);

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

		// render_instances(command_buffer, render_pass, &meshes[0], pipeline_3d);

		UniformData uniform_data = {
			.near_plane = camera->near_plane,
			.far_plane = camera->far_plane,
			.ambient_light = scene->ambient_light,
			.num_lights = num_lights,
			.camera_position = get_position(scene->camera)
		};
		SDL_PushGPUFragmentUniformData(command_buffer, 0, &uniform_data, sizeof(UniformData));
		SDL_PushGPUFragmentUniformData(command_buffer, 1, &lights, sizeof(LightData) * num_lights);

		for (int i = 0; i < resources.meshes_size; i++) {
			render_instances(command_buffer, render_pass, &resources.meshes[i], PIPELINE_3D_TEXTURED);
			resources.meshes[i].num_instances = 0;
		}

		SDL_EndGPURenderPass(render_pass);
		num_lights = 0;
	}

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


void add_render_instance(int mesh_index, Matrix4 transform, int texture_index, int material_index) {
	MeshData* render_data = &resources.meshes[mesh_index];

	if (render_data->num_instances >= render_data->max_instances) {
		LOG_INFO("Buffer full, resizing...");
		render_data->instance_buffer = double_buffer_size(
			render_data->instance_buffer,
			sizeof(InstanceData) * render_data->max_instances
		);
		render_data->instance_transfer_buffer = double_transfer_buffer_size(
			render_data->instance_transfer_buffer,
			sizeof(InstanceData) * render_data->max_instances
		);
		render_data->max_instances *= 2;
		LOG_INFO("New buffer size: %d", render_data->max_instances);
	}

	// TODO: Map the transfer buffer only once per frame
	InstanceData* transforms = SDL_MapGPUTransferBuffer(app.gpu_device, render_data->instance_transfer_buffer, false);

	InstanceData instance_data = {
		.transform = transpose4(transform),
		.material = resources.materials[material_index],
		.texture_index = texture_index,
	};
	transforms[render_data->num_instances] = instance_data;
	render_data->num_instances++;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, render_data->instance_transfer_buffer);
}
