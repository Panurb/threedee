#pragma once

#include <SDL3/SDL_gpu.h>

#include "component.h"


typedef struct {
    SDL_GPUTexture* depth_texture;
    SDL_GPUBuffer* frame_buffer;
    Matrix4 light_view_projection;
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
