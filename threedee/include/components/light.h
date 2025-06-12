#pragma once

#include <SDL3/SDL_gpu.h>


#define SHADOW_MAP_RESOLUTION 4096
#define MAX_LIGHTS 32  // Should match array size in phong shader


typedef struct {
    SDL_GPUTexture* texture;  // Only used for debugging
    SDL_GPUTexture* depth_texture;
    Matrix4 projection_view_matrix;
} ShadowMap;


typedef struct {
    Color diffuse_color;
    Color specular_color;
    float range;
    float intensity;
    ShadowMap shadow_map;
} LightComponent;


void LightComponent_add(int entity, Color color);


void LightComponent_remove(int entity);
