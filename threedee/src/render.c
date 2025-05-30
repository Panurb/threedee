#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"

#include <component.h>
#include <scene.h>
#include <settings.h>

#include "app.h"
#include "util.h"


static SDL_GPUGraphicsPipeline* pipeline_2d = NULL;
static SDL_GPUGraphicsPipeline* pipeline_3d = NULL;

static SDL_GPUDevice* device = NULL;
static SDL_GPUCommandBuffer* command_buffer = NULL;
static SDL_GPUTexture* depth_stencil_texture = NULL;

static RenderData render_datas[2];


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
	SDL_GPUShader* vertex_shader = load_shader(device, "triangle.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(device, "SolidColor.frag", 0, 0, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(device, app.window),
			}},
		},
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

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


SDL_GPUGraphicsPipeline* create_render_pipeline_3d() {
	SDL_GPUShader* vertex_shader = load_shader(device, "triangle.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(device, "SolidColorDepth.frag", 0, 1, 0, 0);
	if (!fragment_shader) {
		LOG_ERROR("Failed to load fragment shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
				.format = SDL_GetGPUSwapchainTextureFormat(device, app.window),
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

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);

	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

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
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * render_mode.num_vertices,
        }
    );

    render_mode.num_indices = 6;
    render_mode.index_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * render_mode.num_indices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
        }
    );

    render_mode.instance_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(Matrix4) * render_mode.max_instances,
        }
    );

    render_mode.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Matrix4),
        }
    );

    Vertex* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

    transfer_data[0] = (Vertex) { {-0.5f, -0.5f, 0.0f}, {255, 0, 0, 255}};
    transfer_data[1] = (Vertex) { {0.5f, -0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[2] = (Vertex) { {0.5f, 0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[3] = (Vertex) { {-0.5f, 0.5f, 0.0f}, {0, 0, 255, 255} };

    Uint16* index_data = (Uint16*) &transfer_data[render_mode.num_vertices];
    const Uint16 indices[6] = { 0, 1, 2, 0, 2, 3 };
    SDL_memcpy(index_data, indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(device);
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
            .size = sizeof(Vertex) * render_mode.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(Vertex) * render_mode.num_vertices
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
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

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
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * render_mode.num_vertices,
        }
    );

    render_mode.num_indices = 36;
    render_mode.index_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * render_mode.num_indices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
        }
    );

    render_mode.instance_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = sizeof(Matrix4) * render_mode.max_instances,
        }
    );

    render_mode.instance_transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Matrix4),
        }
    );

    Vertex* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

    transfer_data[0] = (Vertex) { {-0.5f, -0.5f, -0.5f}, {255, 0, 0, 255}};
	transfer_data[1] = (Vertex) { {0.5f, -0.5f, -0.5f}, {255, 255, 0, 255}};
	transfer_data[2] = (Vertex) { {0.5f, 0.5f, -0.5f}, {0, 255, 0, 255}};
	transfer_data[3] = (Vertex) { {-0.5f, 0.5f, -0.5f}, {0, 255, 255, 255}};
	transfer_data[4] = (Vertex) { {-0.5f, -0.5f, 0.5f}, {255, 0, 255, 255}};
	transfer_data[5] = (Vertex) { {0.5f, -0.5f, 0.5f}, {255, 255, 255, 255}};
	transfer_data[6] = (Vertex) { {0.5f, 0.5f, 0.5f}, {128, 128, 128, 255}};
	transfer_data[7] = (Vertex) { {-0.5f, 0.5f, 0.5f}, {64, 64, 64, 255} };

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

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(device);
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
            .size = sizeof(Vertex) * render_mode.num_vertices
        },
        false
    );

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(Vertex) * render_mode.num_vertices
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
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	return render_mode;
}


void init_render() {
	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
	SDL_ClaimWindowForGPUDevice(device, app.window);

	pipeline_2d = create_render_pipeline_2d();
	pipeline_3d = create_render_pipeline_3d();
	render_datas[RENDER_QUAD] = create_render_mode_quad();
	render_datas[RENDER_CUBE] = create_render_mode_cube();

	SDL_GPUTextureCreateInfo depth_stencil_texture_info = {
		.width = game_settings.width,
		.height = game_settings.height,
		.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.layer_count_or_depth = 1,
		.num_levels = 1
	};
	depth_stencil_texture = SDL_CreateGPUTexture(device, &depth_stencil_texture_info);
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

	SDL_DrawGPUIndexedPrimitives(render_pass, render_mode->num_indices, render_mode->num_instances,  0, 0, 0);

	render_mode->num_instances = 0;
}


void render() {
	command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (!command_buffer) {
		LOG_ERROR("Failed to acquire GPU command buffer: %s", SDL_GetError());
		return;
	}

	SDL_GPUTexture* swapchain_texture;
	SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, app.window, &swapchain_texture, NULL, NULL);

	if (swapchain_texture) {
		add_render_instance(RENDER_CUBE, transform_matrix(vec3(0.0f, 0.0f, 0.0f), (Rotation) { 0 }, ones3()));
		add_render_instance(RENDER_CUBE, transform_matrix(vec3(2.0f, 0.0f, 2.0f), (Rotation) { 0 }, ones3()));

		Matrix4 view_matrix = transform_inverse(get_transform(scene->camera));
		// Need to shift so rotation happens around the center of the camera
		// Otherwise the camera "orbits"
		view_matrix._34 += 1.0f;
		LOG_INFO("Camera view matrix:");
		matrix4_print(view_matrix);
		Matrix4 projection_matrix = CameraComponent_get(scene->camera)->projection_matrix;
		LOG_INFO("Camera projection matrix:");
		matrix4_print(projection_matrix);
		Matrix4 projection_view_matrix = transpose4(matrix4_mult(projection_matrix, view_matrix));
		LOG_INFO("Camera projection view matrix:");
		matrix4_print(transpose4(projection_view_matrix));
		SDL_PushGPUVertexUniformData(command_buffer, 0, &projection_view_matrix, sizeof(Matrix4));
		SDL_PushGPUFragmentUniformData(command_buffer, 0, (float[]) { 0.1f, 1000.0f }, 8);

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
			device,
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

		SDL_ReleaseGPUBuffer(device, old_buffer);
		render_data->max_instances *= 2;
		LOG_INFO("New buffer size: %d", render_data->max_instances);
	}

	// TODO: Map the transfer buffer only once per frame
	Matrix4* transforms = SDL_MapGPUTransferBuffer(device, render_data->instance_transfer_buffer, false);

	transforms[render_data->num_instances] = transpose4(transform);
	render_data->num_instances = render_data->num_instances + 1;

	SDL_UnmapGPUTransferBuffer(device, render_data->instance_transfer_buffer);
}
