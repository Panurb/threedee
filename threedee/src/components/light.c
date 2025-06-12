#include <stdlib.h>

#include "component.h"
#include "components/light.h"

#include <app.h>

#include "scene.h"



void LightComponent_add(Entity entity, Color color) {
    LightComponent* light = malloc(sizeof(LightComponent));
    light->diffuse_color = color;
    light->specular_color = color;
    light->range = 20.0f;
    light->intensity = 1.0f;
    light->shadow_map.texture = SDL_CreateGPUTexture(
        app.gpu_device,
        &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .width = SHADOW_MAP_RESOLUTION,
            .height = SHADOW_MAP_RESOLUTION,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
        }
    );
    light->shadow_map.depth_texture = SDL_CreateGPUTexture(
        app.gpu_device,
        &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
            .width = SHADOW_MAP_RESOLUTION,
            .height = SHADOW_MAP_RESOLUTION,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        }
    );
    light->shadow_map.projection_view_matrix = matrix4_id();

    scene->components->light[entity] = light;
}


void LightComponent_remove(Entity entity) {
    LightComponent* light = scene->components->light[entity];
    if (light) {
        SDL_ReleaseGPUTexture(app.gpu_device, light->shadow_map.depth_texture);
        free(light);
        scene->components->light[entity] = NULL;
    }
}
