#pragma once

#include <SDL3/SDL_gpu.h>

#include "linalg.h"
#include "util.h"


typedef struct RenderMode {
	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUBuffer* vertex_buffer;
	int num_vertices;
	SDL_GPUBuffer* index_buffer;
	int num_indices;
	SDL_GPUBuffer* instance_buffer;
	int num_instances;
	int max_instances;
	SDL_GPUTransferBuffer* instance_transfer_buffer;
} RenderMode;


typedef struct RenderData {
	SDL_GPUGraphicsPipeline* pipeline_triangle;
} RenderData;


typedef struct Vertex {
    Vector3 position;
    Color color;
} Vertex;


SDL_GPUGraphicsPipeline* create_render_pipeline_triangle();

RenderMode create_render_mode_quad();

void render(SDL_GPUCommandBuffer* gpu_command_buffer, SDL_GPURenderPass* render_pass, RenderMode* render_mode);

void add_render_instance(RenderMode* render_mode, Matrix4 transform);
