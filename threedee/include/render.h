#pragma once

#include <SDL3/SDL_gpu.h>

#include "resources.h"
#include "components/light.h"
#include "linalg.h"
#include "util.h"


typedef struct PositionColorVertex {
    Vector3 position;
    Color color;
} PositionColorVertex;


typedef struct PositionTextureVertex {
	Vector3 position;
	Vector2 uv;
	Vector3 normal;
	Vector3 tangent;
} PositionTextureVertex;


typedef struct UniformData {
	float near_plane;
	float far_plane;
	float ambient_light;
	int num_lights;
	Vector3 camera_position;
	int shadow_map_resolution;
	FloatColor fog_color;
	float fog_start;
	float fog_end;
} UniformData;


typedef struct {
	Matrix4 transform;
	Material material;
	int texture_index;
	Visibility visiblity;
	float _pad[2];
} InstanceData;


typedef struct {
	Matrix4 transform;
	FloatColor color;
} InstanceColorData;


typedef struct Matrix2x3 {
	float _11, _12, _13, _pad1;
	float _21, _22, _23, _pad2;
} Matrix2x3;


typedef struct InstanceColorData2D {
	Matrix2x3 transform;
	FloatColor color;
} InstanceColorData2D;


typedef struct {
	Vector3 position;
	int visibility_mask;
	Vector3 direction;
	float cutoff_cos;
	Vector3 diffuse_color;
	float _pad2;
	Vector3 specular_color;
	float _pad3;
	Matrix4 projection_view_matrix;
} LightData;


typedef struct {
	Matrix4 projection_view_matrix;
	Visibility visibility_mask;
} ShadowUniformData;


typedef struct PostProcessingUniformData {
	float near_plane;
	float far_plane;
} PostProcessingUniformData;


typedef struct DepthOfFieldUniformData {
	float near_plane;
	float far_plane;
	float focal_distance;
	float focal_range;
	float screen_size[2];
	bool vertical;
} DepthOfFieldUniformData;


void init_render();

void apply_render_settings();

void render();

SDL_GPUBuffer* double_buffer_size(SDL_GPUBuffer* buffer, int size);

SDL_GPUTransferBuffer* double_transfer_buffer_size(SDL_GPUTransferBuffer* transfer_buffer, int size);

void add_light(Entity entity);

void render_mesh(Matrix4 transform, int mesh_index, int texture_index, int material_index, int light_index);

void render_triangle(Vector3 a, Vector3 b, Vector3 c, Color color);

void render_line(Vector3 start, Vector3 end, float thickness, Color color);

void render_circle(Vector3 center, float radius, int segments, Color color);

void render_sphere(Vector3 center, float radius, int segments, Color color);

void render_quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color);

void render_arrow(Vector3 start, Vector3 end, float thickness, Color color);

void render_plane(Plane plane, Color color);

void draw_triangle_2d(Vector2 a, Vector2 b, Vector2 c, Color color);

void draw_circle_2d(Vector2 center, float radius, Color color);
