#pragma once

#include <SDL3/SDL_gpu.h>


#define SHADOW_MAP_RESOLUTION 2048
#define MAX_LIGHTS 32  // Should match array size in phong shader


typedef enum {
    LIGHT_SPOT,
    LIGHT_DIRECTIONAL
} LightShape;


typedef enum {
    LIGHT_NORMAL = 1 << 0,
    LIGHT_UV = 1 << 1
} LightType;


typedef struct {
    SDL_GPUTexture* depth_texture;
    Matrix4 projection_view_matrix;
} ShadowMap;


typedef struct {
    LightShape shape;
    LightType type;
    float fov;
    float range;
    float intensity;
    Color color;
} LightParameters;


typedef struct {
    LightType type;
    float fov;
    Color diffuse_color;
    Color specular_color;
    float range;
    float intensity;
    Matrix4 projection_matrix;
    ShadowMap shadow_map;
} LightComponent;


LightComponent* LightComponent_add(int entity, LightParameters parameters);


void LightComponent_remove(int entity);
