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


static SDL_GPUGraphicsPipeline* pipeline_2d = NULL;
static SDL_GPUGraphicsPipeline* pipeline_3d = NULL;
static SDL_GPUGraphicsPipeline* pipeline_3d_textured = NULL;
static SDL_GPUCommandBuffer* command_buffer = NULL;
static SDL_GPUTexture* depth_stencil_texture = NULL;
static SDL_GPUTexture* texture = NULL;

static RenderData render_datas[3];


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

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "texture_depth.frag", 1, 1, 0, 0);
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
			.num_vertex_attributes = 3,
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


RenderData create_render_mode_quad() {
	RenderData render_mode = {
		.pipeline = pipeline_2d,
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


RenderData create_render_mode_cube() {
	RenderData render_mode = {
		.pipeline = pipeline_3d,
		.max_instances = 256,
		.num_instances = 0
	};

	render_mode.num_vertices = 8;
    render_mode.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionColorVertex) * render_mode.num_vertices,
        }
    );

    render_mode.num_indices = 36;
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

    transfer_data[0] = (PositionColorVertex) { {-0.5f, -0.5f, -0.5f}, {255, 0, 0, 255}};
	transfer_data[1] = (PositionColorVertex) { {0.5f, -0.5f, -0.5f}, {255, 255, 0, 255}};
	transfer_data[2] = (PositionColorVertex) { {0.5f, 0.5f, -0.5f}, {0, 255, 0, 255}};
	transfer_data[3] = (PositionColorVertex) { {-0.5f, 0.5f, -0.5f}, {0, 255, 255, 255}};
	transfer_data[4] = (PositionColorVertex) { {-0.5f, -0.5f, 0.5f}, {255, 0, 255, 255}};
	transfer_data[5] = (PositionColorVertex) { {0.5f, -0.5f, 0.5f}, {255, 255, 255, 255}};
	transfer_data[6] = (PositionColorVertex) { {0.5f, 0.5f, 0.5f}, {128, 128, 128, 255}};
	transfer_data[7] = (PositionColorVertex) { {-0.5f, 0.5f, 0.5f}, {64, 64, 64, 255} };

	// Front-facing triangle should go clockwise
    Uint16* index_data = (Uint16*) &transfer_data[render_mode.num_vertices];
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


RenderData create_render_mode_cube_textured() {
	RenderData render_mode = {
		.pipeline = pipeline_3d_textured,
		.max_instances = 256,
		.num_instances = 0
	};

	render_mode.num_vertices = 24;
    render_mode.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex) * render_mode.num_vertices,
        }
    );

    render_mode.num_indices = 36;
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
            .size = sizeof(PositionTextureVertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
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

    PositionTextureVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

	// Front face
	Vector3 n = {0.0f, 0.0f, 1.0f};
	transfer_data[0] = (PositionTextureVertex) { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, n };
	transfer_data[1] = (PositionTextureVertex) { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, n };
	transfer_data[2] = (PositionTextureVertex) { { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, n };
	transfer_data[3] = (PositionTextureVertex) { {-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, n };

	// Back face
	n = (Vector3) {0.0f, 0.0f, -1.0f};
	transfer_data[4] = (PositionTextureVertex) { {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, n };
	transfer_data[5] = (PositionTextureVertex) { { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, n };
	transfer_data[6] = (PositionTextureVertex) { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, n };
	transfer_data[7] = (PositionTextureVertex) { {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, n };

	// Left face
	n = (Vector3) {-1.0f, 0.0f, 0.0f};
	transfer_data[8] = (PositionTextureVertex) { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, n };
	transfer_data[9] = (PositionTextureVertex) { {-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, n };
	transfer_data[10] = (PositionTextureVertex) { {-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, n };
	transfer_data[11] = (PositionTextureVertex) { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, n };

	// Right face
	n = (Vector3) {1.0f, 0.0f, 0.0f};
	transfer_data[12] = (PositionTextureVertex) { { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, n };
	transfer_data[13] = (PositionTextureVertex) { { 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, n };
	transfer_data[14] = (PositionTextureVertex) { { 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, n };
	transfer_data[15] = (PositionTextureVertex) { { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, n };

	// Top face
	n = (Vector3) {0.0f, 1.0f, 0.0f};
	transfer_data[16] = (PositionTextureVertex) { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, n };
	transfer_data[17] = (PositionTextureVertex) { { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, n };
	transfer_data[18] = (PositionTextureVertex) { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, n };
	transfer_data[19] = (PositionTextureVertex) { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, n };

	// Bottom face
	n = (Vector3) {0.0f, -1.0f, 0.0f};
	transfer_data[20] = (PositionTextureVertex) { {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, n };
	transfer_data[21] = (PositionTextureVertex) { { 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, n };
	transfer_data[22] = (PositionTextureVertex) { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, n };
	transfer_data[23] = (PositionTextureVertex) { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, n };

    Uint16* index_data = (Uint16*) &transfer_data[render_mode.num_vertices];
	// Indices for 6 faces, 2 triangles per face, counter-clockwise winding
	const Uint16 indices[36] = {
		// Front face
		0, 1, 2, 0, 2, 3,
		// Back face
		4, 6, 5, 4, 7, 6,
		// Left face
		8, 9, 10, 8, 10, 11,
		// Right face
		12, 14, 13, 12, 15, 14,
		// Top face
		16, 18, 17, 16, 19, 18,
		// Bottom face
		20, 21, 22, 20, 22, 23
	};
    SDL_memcpy(index_data, indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

	// LOAD TEXTURE
	SDL_Surface* image_data = IMG_Load("data/images/brick_tile.png");
	if (!image_data) {
		LOG_ERROR("Failed to load texture image: %s", SDL_GetError());
	}

	texture = SDL_CreateGPUTexture(
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

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = render_mode.vertex_buffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex) * render_mode.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(PositionTextureVertex) * render_mode.num_vertices
        },
        &(SDL_GPUBufferRegion) {
            .buffer = render_mode.index_buffer,
            .offset = 0,
            .size = sizeof(Uint16) * render_mode.num_indices
        },
        false
    );

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
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);
	SDL_DestroySurface(image_data);

	render_mode.sampler = SDL_CreateGPUSampler(
		app.gpu_device,
		&(SDL_GPUSamplerCreateInfo){
			.min_filter = SDL_GPU_FILTER_LINEAR,
			.mag_filter = SDL_GPU_FILTER_LINEAR,
			.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
			.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
			.enable_anisotropy = true,
			.max_anisotropy = 16
		}
	);

	return render_mode;
}


void init_render() {
	app.gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	SDL_ClaimWindowForGPUDevice(app.gpu_device, app.window);

	pipeline_2d = create_render_pipeline_2d();
	pipeline_3d = create_render_pipeline_3d();
	pipeline_3d_textured = create_render_pipeline_3d_textured();

	render_datas[RENDER_QUAD] = create_render_mode_quad();
	render_datas[RENDER_CUBE] = create_render_mode_cube();
	render_datas[RENDER_CUBE_TEXTURED] = create_render_mode_cube_textured();

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
}


void render_instances(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass,
			RenderData* render_mode) {
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(gpu_command_buffer);
	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = render_mode->instance_transfer_buffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = render_mode->instance_buffer,
			.offset = 0,
			.size = sizeof(Matrix4) * render_mode->num_instances
		},
		true
	);
	SDL_EndGPUCopyPass(copy_pass);

	SDL_BindGPUGraphicsPipeline(render_pass, render_mode->pipeline);
	SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding) { .buffer = render_mode->vertex_buffer, .offset = 0 }, 1);
	SDL_BindGPUIndexBuffer(
		render_pass, &(SDL_GPUBufferBinding) { .buffer = render_mode->index_buffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT
	);
	SDL_BindGPUVertexStorageBuffers(render_pass, 0, &render_mode->instance_buffer, 1);
	if (render_mode->sampler) {
		SDL_BindGPUFragmentSamplers(
			render_pass,
			0,
			&(SDL_GPUTextureSamplerBinding){
				.texture = resources.textures[0],
				.sampler = render_mode->sampler,
			},
			1);
	}

	SDL_DrawGPUIndexedPrimitives(render_pass, render_mode->num_indices, render_mode->num_instances, 0, 0, 0);

	render_mode->num_instances = 0;
}


void render() {
	static float angle = 0.0f;
	angle += 0.01f;

	command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
	if (!command_buffer) {
		LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
		return;
	}

	SDL_GPUTexture* swapchain_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window, &swapchain_texture, NULL, NULL);

	if (swapchain_texture) {
		add_render_instance(RENDER_CUBE, transform_matrix(vec3(0.0f, 0.0f, 0.0f), (Rotation) { 0 }, ones3()));
		add_render_instance(RENDER_CUBE, transform_matrix(vec3(2.0f, 0.0f, 2.0f), (Rotation) { 0 }, ones3()));
		add_render_instance(RENDER_CUBE_TEXTURED, transform_matrix(vec3(0.0f, 0.0f, 2.0f), (Rotation) { angle }, vec3(2.0f, 1.0f, 1.0f)));

		CameraComponent* camera = CameraComponent_get(scene->camera);
		Matrix4 view_matrix = transform_inverse(get_transform(scene->camera));
		// Need to shift so rotation happens around the center of the camera
		// Otherwise the camera "orbits"
		view_matrix._34 += 1.0f;
		Matrix4 projection_matrix = camera->projection_matrix;
		Matrix4 projection_view_matrix = transpose4(matrix4_mult(projection_matrix, view_matrix));

		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_view_matrix, sizeof(Matrix4));
		SDL_PushGPUFragmentUniformData(command_buffer, 0, (float[]) { camera->near_plane, camera->far_plane }, 8);

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
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.texture = depth_stencil_texture,
				.cycle = true,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE,
				.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
				.stencil_store_op = SDL_GPU_STOREOP_STORE,
			}
		);

		render_instances(command_buffer, render_pass, &render_datas[RENDER_CUBE]);

		UniformData uniform_data = {
			.near_plane = camera->near_plane,
			.far_plane = camera->far_plane,
			.light_direction = { 0.0f, -1.0f, -1.0f }
		};
		SDL_PushGPUFragmentUniformData(command_buffer, 0, &uniform_data, sizeof(UniformData));
		render_instances(command_buffer, render_pass, &render_datas[RENDER_CUBE_TEXTURED]);

		SDL_EndGPURenderPass(render_pass);
	}

	SDL_SubmitGPUCommandBuffer(command_buffer);
	command_buffer = NULL;
}


void add_render_instance(RenderMode render_mode, Matrix4 transform) {
	RenderData* render_data = &render_datas[render_mode];

	if (render_data->num_instances >= render_data->max_instances) {
		LOG_INFO("Buffer full, resizing...");
		SDL_GPUBuffer* old_buffer = render_data->instance_buffer;

		render_data->instance_buffer = SDL_CreateGPUBuffer(
			app.gpu_device,
			&(SDL_GPUBufferCreateInfo){
				.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
				.size = sizeof(Matrix4) * 2 * render_data->max_instances,
			}
		);

		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
		SDL_CopyGPUBufferToBuffer(
			copy_pass,
			&(SDL_GPUBufferLocation) {
				.buffer = render_data->instance_buffer,
				.offset = 0
			},
			&(SDL_GPUBufferLocation) {
				.buffer = render_data->instance_buffer,
				.offset = 0
			},
			sizeof(Matrix4) * render_data->max_instances,
			false
		);
		SDL_EndGPUCopyPass(copy_pass);

		SDL_ReleaseGPUBuffer(app.gpu_device, old_buffer);
		render_data->max_instances *= 2;
		LOG_INFO("New buffer size: %d", render_data->max_instances);
	}

	// TODO: Map the transfer buffer only once per frame
	Matrix4* transforms = SDL_MapGPUTransferBuffer(app.gpu_device, render_data->instance_transfer_buffer, false);

	transforms[render_data->num_instances] = transpose4(transform);
	render_data->num_instances = render_data->num_instances + 1;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, render_data->instance_transfer_buffer);
}
