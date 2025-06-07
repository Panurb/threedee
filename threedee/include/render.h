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
	float _pad;
	Vector3 camera_position;
	float _pad2;
	Vector3 light_position;
} UniformData;


typedef struct {
	Matrix4 transform;
	Rect texture_rect;
	Material material;
} InstanceData;


typedef struct {
	Vector3 position;
	Vector3 diffuse_color;
	Vector3 specular_color;
} LightData;


void init_render();

void render();

void add_render_instance(int mesh_index, Matrix4 transform, int texture_index, int material_index);
