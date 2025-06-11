#pragma once

#include <SDL3/SDL_gpu.h>

#include "component.h"


typedef struct {
    SDL_GPUTexture* depth_texture;
    Matrix4 view_projection_matrix;
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
