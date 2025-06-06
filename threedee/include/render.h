#pragma once

#include <SDL3/SDL_gpu.h>

#include "linalg.h"
#include "util.h"


typedef struct MeshData{
	SDL_GPUBuffer* vertex_buffer;
	int num_vertices;
	SDL_GPUBuffer* index_buffer;
	int num_indices;
	SDL_GPUBuffer* instance_buffer;
	int num_instances;
	int max_instances;
	SDL_GPUTransferBuffer* instance_transfer_buffer;
	SDL_GPUSampler* sampler;
} MeshData;


typedef enum Mesh {
	MESH_QUAD,
	MESH_CUBE,
} Mesh;


typedef struct PositionColorVertex {
    Vector3 position;
    Color color;
} PositionColorVertex;


typedef struct PositionTextureVertex {
	Vector3 position;
	Vector2 uv;
	Vector3 normal;
} PositionTextureVertex;


typedef struct UniformData {
	float near_plane;
	float far_plane;
	float _pad[2];
	Vector3 camera_position;
	float _pad2;
	Vector3 light_position;
} UniformData;


typedef struct {
	Matrix4 transform;
	Rect texture_rect;
} InstanceData;


void init_render();

void render();

void add_render_instance(int mesh_index, Matrix4 transform, int texture_index);
