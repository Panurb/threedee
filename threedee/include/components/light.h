#pragma once

#include <SDL3/SDL_gpu.h>


#define SHADOW_MAP_RESOLUTION 512
#define MAX_LIGHTS 32


typedef enum {
    LIGHT_SPOT,
    LIGHT_DIRECTIONAL
} LightShape;


typedef enum {
    VISIBILITY_NORMAL = 1 << 0,
    VISIBILITY_UV = 1 << 1,
    VISIBILITY_ALL = VISIBILITY_NORMAL | VISIBILITY_UV,
} Visibility;


typedef struct {
    bool disabled;
    LightShape shape;
    Visibility visibility_mask;
    float fov;
    float range;
    float intensity;
    Color color;
} LightParameters;


typedef struct {
    bool disabled;
    Visibility visibility_mask;
    float fov;
    Color diffuse_color;
    Color specular_color;
    float range;
    float intensity;
    float flicker;
    Matrix4 projection_matrix;
} LightComponent;


LightComponent* LightComponent_add(int entity, LightParameters parameters);


void LightComponent_remove(int entity);
