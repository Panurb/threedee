#include <stdlib.h>

#include "component.h"
#include "components/light.h"

#include <app.h>
#include <stdio.h>

#include "scene.h"



LightComponent* LightComponent_add(Entity entity, LightParameters params) {
    LightComponent* light = malloc(sizeof(LightComponent));
    light->type = params.type ? params.type : LIGHT_NORMAL;
    light->fov = params.fov ? params.fov : 90.0f;
    light->diffuse_color = params.color;
    light->specular_color = params.color;
    light->range = params.range ? params.range : 10.0f;
    light->intensity = params.intensity ? params.intensity : 1.0f;
    float half_size = light->range * tanf(to_radians(light->fov) * 0.5f);

    switch (params.shape) {
        case LIGHT_SPOT:
            light->projection_matrix = perspective_projection_matrix(
                to_radians(light->fov),
                (float) SHADOW_MAP_RESOLUTION / (float) SHADOW_MAP_RESOLUTION,
                0.1f,
                light->range + half_size
            );
            break;
        case LIGHT_DIRECTIONAL:
            light->projection_matrix = orthographic_projection_matrix(
                -half_size, half_size,
                -half_size, half_size,
                0.1f,
                light->range + half_size
            );
            break;
        default:
            LOG_ERROR("Unknown light type: %d", params.shape);
            free(light);
            return NULL;
    }

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

    return light;
}


void LightComponent_remove(Entity entity) {
    LightComponent* light = scene->components->light[entity];
    if (light) {
        SDL_ReleaseGPUTexture(app.gpu_device, light->shadow_map.depth_texture);
        free(light);
        scene->components->light[entity] = NULL;
    }
}
