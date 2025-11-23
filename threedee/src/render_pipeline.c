#include <stdio.h>

#include <SDL3/SDL_gpu.h>

#include "render_pipeline.h"
#include "render.h"
#include "app.h"
#include "linalg.h"
#include "util.h"


static SDL_GPUShader* shaders[SHADER_COUNT] = { 0 };


static const SDL_GPUColorTargetBlendState BLEND_STATE = {
	.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	.color_blend_op = SDL_GPU_BLENDOP_ADD,
	.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
	.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
	.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	.enable_blend = true
};


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
	shaders[SHADER_VERTEX_POSITION_TEXTURE] = load_shader(app.gpu_device, "position_texture.vert", 0, 2, 1, 0);
	shaders[SHADER_VERTEX_TEXT] = load_shader(app.gpu_device, "text.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_SHADOW_DEPTH] = load_shader(app.gpu_device, "shadow_depth.vert", 0, 2, 1, 0);
	shaders[SHADER_VERTEX_POST_PROCESSING] = load_shader(app.gpu_device, "post_processing.vert", 0, 0, 0, 0);
	shaders[SHADER_VERTEX_BILLBOARD] = load_shader(app.gpu_device, "billboard.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_LINE] = load_shader(app.gpu_device, "line.vert", 0, 1, 1, 0);
	shaders[SHADER_VERTEX_CUBE] = load_shader(app.gpu_device, "cube.vert", 0, 1, 1, 0);
	shaders[SHADER_FRAGMENT_SOLID_COLOR] = load_shader(app.gpu_device, "solid_color.frag", 0, 0, 0, 0);
	shaders[SHADER_FRAGMENT_SOLID_COLOR_DEPTH] = load_shader(app.gpu_device, "solid_color_depth.frag", 0, 0, 0, 0);
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
			.cull_mode = SDL_GPU_CULLMODE_NONE,  // This is only for debug, no need to optimize
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


SDL_GPUGraphicsPipeline* create_render_pipeline_cube() {
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
		.vertex_shader = shaders[SHADER_VERTEX_CUBE],
		.fragment_shader = shaders[SHADER_FRAGMENT_PHONG],
	};

	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(app.gpu_device, &pipeline_info);

	if (!pipeline) {
		LOG_ERROR("Failed to create graphics pipeline: %s", SDL_GetError());
	}

	return pipeline;
}

void create_pipelines() {
	pipelines[PIPELINE_2D] = create_render_pipeline_2d();
	pipelines[PIPELINE_TEXT] = create_render_pipeline_text();
	pipelines[PIPELINE_3D] = create_render_pipeline_3d();
	pipelines[PIPELINE_3D_TEXTURED] = create_render_pipeline_3d_textured();
	pipelines[PIPELINE_SHADOW_DEPTH] = create_render_pipeline_shadow_depth();
	pipelines[PIPELINE_POST_PROCESSING] = create_render_pipeline_post_processing();
	pipelines[PIPELINE_DEPTH_OF_FIELD] = create_render_pipeline_depth_of_field();
	pipelines[PIPELINE_BILLBOARD] = create_render_pipeline_billboard();
	pipelines[PIPELINE_LINE] = create_render_pipeline_line();
	pipelines[PIPELINE_CUBE] = create_render_pipeline_cube();
}


void destroy_pipelines() {
	for (int i = 0; i < PIPELINE_COUNT; i++) {
		if (pipelines[i]) {
			SDL_ReleaseGPUGraphicsPipeline(app.gpu_device, pipelines[i]);
			pipelines[i] = NULL;
		}
	}

	for (int i = 0; i < SHADER_COUNT; i++) {
		if (shaders[i]) {
			SDL_ReleaseGPUShader(app.gpu_device, shaders[i]);
			shaders[i] = NULL;
		}
	}
}
