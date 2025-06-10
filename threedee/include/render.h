#pragma once

#include <resources.h>
#include <SDL3/SDL_gpu.h>

#include "linalg.h"
#include "util.h"


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
	float ambient_light;
	int num_lights;
	Vector3 camera_position;
} UniformData;


typedef struct {
	Matrix4 transform;
	Material material;
	int texture_index;
	float _pad[3];
} InstanceData;


typedef struct {
	Vector3 position;
	float _pad;
	Vector3 diffuse_color;
	float _pad2;
	Vector3 specular_color;
	float _pad3;
} LightData;


void init_render();

void render();

void add_render_instance(int mesh_index, Matrix4 transform, int texture_index, int material_index);

SDL_GPUBuffer* double_buffer_size(SDL_GPUBuffer* buffer, int size);

SDL_GPUTransferBuffer* double_transfer_buffer_size(SDL_GPUTransferBuffer* transfer_buffer, int size);
