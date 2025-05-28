#pragma once

#include <SDL3/SDL_gpu.h>

#include "linalg.h"
#include "util.h"


typedef struct RenderData {
	SDL_GPUGraphicsPipeline* pipeline;
	SDL_GPUBuffer* vertex_buffer;
	int num_vertices;
	SDL_GPUBuffer* index_buffer;
	int num_indices;
	SDL_GPUBuffer* instance_buffer;
	int num_instances;
	int max_instances;
	SDL_GPUTransferBuffer* instance_transfer_buffer;
} RenderData;


typedef enum RenderMode {
	RENDER_QUAD,
	RENDER_CUBE,
} RenderMode;


typedef struct Vertex {
    Vector3 position;
    Color color;
} Vertex;


void init_render();

void render();

void add_render_instance(RenderMode render_mode, Matrix4 transform);
