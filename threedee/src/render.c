#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

#include "render.h"
#include "app.h"
#include "util.h"


static SDL_GPUGraphicsPipeline* pipeline_solid = NULL;


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


SDL_GPUGraphicsPipeline* create_render_pipeline_solid() {
	SDL_GPUShader* vertex_shader = load_shader(app.gpu_device, "triangle.vert", 0, 1, 1, 0);
	if (!vertex_shader) {
		LOG_ERROR("Failed to load vertex shader: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShader* fragment_shader = load_shader(app.gpu_device, "SolidColor.frag", 0, 0, 0, 0);
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

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	SDL_ReleaseGPUShader(app.gpu_device, vertex_shader);
	SDL_ReleaseGPUShader(app.gpu_device, fragment_shader);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}


RenderMode create_render_mode_quad() {
	RenderMode render_mode = {
		.pipeline = pipeline_solid,
		.max_instances = 256,
		.num_instances = 0
	};

	render_mode.num_vertices = 4;
    render_mode.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * render_mode.num_vertices,
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
            .size = sizeof(Vertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
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

    Vertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vertex) { {-0.5f, -0.5f, 0.0f}, {255, 0, 0, 255}};
    transfer_data[1] = (Vertex) { {0.5f, -0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[2] = (Vertex) { {0.5f, 0.5f, 0.0f}, {0, 255, 0, 255}};
    transfer_data[3] = (Vertex) { {-0.5f, 0.5f, 0.0f}, {0, 0, 255, 255} };

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
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return render_mode;
}


RenderMode create_render_mode_cube() {
	RenderMode render_mode = {
		.pipeline = pipeline_solid,
		.max_instances = 256,
		.num_instances = 0
	};

	render_mode.num_vertices = 8;
    render_mode.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vertex) * render_mode.num_vertices,
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
            .size = sizeof(Vertex) * render_mode.num_vertices + sizeof(Uint16) * render_mode.num_indices,
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

    Vertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vertex) { {-0.5f, -0.5f, -0.5f}, {255, 0, 0, 255}};
	transfer_data[1] = (Vertex) { {0.5f, -0.5f, -0.5f}, {255, 255, 0, 255}};
	transfer_data[2] = (Vertex) { {0.5f, 0.5f, -0.5f}, {0, 255, 0, 255}};
	transfer_data[3] = (Vertex) { {-0.5f, 0.5f, -0.5f}, {0, 255, 255, 255}};
	transfer_data[4] = (Vertex) { {-0.5f, -0.5f, 0.5f}, {255, 0, 255, 255}};
	transfer_data[5] = (Vertex) { {0.5f, -0.5f, 0.5f}, {255, 255, 255, 255}};
	transfer_data[6] = (Vertex) { {0.5f, 0.5f, 0.5f}, {128, 128, 128, 255}};
	transfer_data[7] = (Vertex) { {-0.5f, 0.5f, 0.5f}, {64, 64, 64, 255} };

    Uint16* index_data = (Uint16*) &transfer_data[render_mode.num_vertices];
    const Uint16 indices[36] = {
		0, 1, 2, 0, 2, 3, // Back face
		4, 5, 6, 4, 6, 7, // Front face
		0, 1, 5, 0, 5, 4, // Bottom face
		2, 3, 7, 2, 7, 6, // Top face
		0, 3, 7, 0, 7, 4, // Left face
		1, 2, 6, 1, 6, 5 // Right face
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
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return render_mode;
}


void init_render() {
	pipeline_solid = create_render_pipeline_solid();
	render_modes.quad = create_render_mode_quad();
	render_modes.cube = create_render_mode_cube();
}


void render(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass,
			RenderMode* render_mode) {
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


void add_render_instance(SDL_GPUCommandBuffer* command_buffer, RenderMode* render_mode, Matrix4 transform) {
	if (render_mode->num_instances >= render_mode->max_instances) {
		LOG_INFO("Buffer full, resizing...");
		SDL_GPUBuffer* old_buffer = render_mode->instance_buffer;

		render_mode->instance_buffer = SDL_CreateGPUBuffer(
			app.gpu_device,
			&(SDL_GPUBufferCreateInfo){
				.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
				.size = sizeof(Matrix4) * 2 * render_mode->max_instances,
			}
		);

		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
		SDL_CopyGPUBufferToBuffer(
			copy_pass,
			&(SDL_GPUBufferLocation) {
				.buffer = render_mode->instance_buffer,
				.offset = 0
			},
			&(SDL_GPUBufferLocation) {
				.buffer = render_mode->instance_buffer,
				.offset = 0
			},
			sizeof(Matrix4) * render_mode->max_instances,
			false
		);
		SDL_EndGPUCopyPass(copy_pass);

		SDL_ReleaseGPUBuffer(app.gpu_device, old_buffer);
		render_mode->max_instances *= 2;
		LOG_INFO("New buffer size: %d", render_mode->max_instances);
	}

	// TODO: Map the transfer buffer only once per frame
	Matrix4* transforms = SDL_MapGPUTransferBuffer(app.gpu_device, render_mode->instance_transfer_buffer, false);

	transforms[render_mode->num_instances] = transform;
	render_mode->num_instances = render_mode->num_instances + 1;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, render_mode->instance_transfer_buffer);
}
