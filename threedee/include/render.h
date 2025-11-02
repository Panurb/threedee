#pragma once

#include "resources.h"
#include "components/light.h"
#include "linalg.h"
#include "util.h"


#define DEPTH_FORMAT SDL_GPU_TEXTUREFORMAT_D32_FLOAT


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


typedef struct PositionTextureVertex2D {
	Vector2 position;
	Vector2 uv;
} PositionTextureVertex2D;


typedef struct CameraData {
	Matrix4 projection_matrix;
	Matrix4 view_matrix;
} CameraData;


typedef struct UniformData {
	float near_plane;
	float far_plane;
	float ambient_light;
	int num_lights;
	Vector3 camera_position;
	int shadow_map_resolution;
	Color fog_color;
	float fog_start;
	float fog_end;
} UniformData;


typedef struct {
	Matrix4 transform;
	int texture_index;
	int emissive_index;
	Vector2 texture_scale;
	Material material;
	Visibility visiblity;
	float _pad[2];
} InstanceData;
static_assert(sizeof(InstanceData) % 16 == 0);


typedef struct {
	Matrix4 transform;
	Color color;
} InstanceColorData;
static_assert(sizeof(InstanceColorData) % 16 == 0);


typedef struct BillboardInstanceData {
	Vector3 position;
	float width;
	float height;
	int texture_index;
	Material material;
	Visibility visiblity;
} BillboardInstanceData;
static_assert(sizeof(BillboardInstanceData) % 16 == 0);


typedef struct Matrix2x3 {
	float _11, _12, _13, _pad1;
	float _21, _22, _23, _pad2;
} Matrix2x3;


typedef struct InstanceColorData2D {
	Matrix2x3 transform;
	Color color;
} InstanceColorData2D;


typedef struct {
	Vector3 position;
	int visibility_mask;
	Vector3 direction;
	float cutoff_cos;
	Vector3 diffuse_color;
	float range;
	Vector3 specular_color;
	float _pad3;
	Matrix4 projection_view_matrix;
} LightData;


typedef struct TextData {
	MeshData mesh;
	Color color;
	Vector2 position;
} TextData;


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

void pre_render();

void render();

void add_light(Entity entity);

void draw_mesh(Matrix4 transform, int mesh_index, int texture_index, Material material, int emissive_index, Visibility visibility, Vector2 texture_scale);

void draw_sprite(Vector3 position, float width, float height, int texture_index);

void render_triangle(Vector3 a, Vector3 b, Vector3 c, Color color);

void render_line(Vector3 start, Vector3 end, float thickness, Color color);

void render_circle(Vector3 center, float radius, int segments, Color color);

void render_sphere(Vector3 center, float radius, int segments, Color color);

void render_quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color);

void render_arrow(Vector3 start, Vector3 end, float thickness, Color color);

void render_plane(Plane plane, Color color);

void draw_triangle_2d(Vector2 a, Vector2 b, Vector2 c, Color color);

void draw_circle_2d(Vector2 center, float radius, Color color);

void draw_text(String string, Vector2 position, float angle, float size, Color color);
