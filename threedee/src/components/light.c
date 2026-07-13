#include <stdlib.h>

#include "component.h"
#include "components/light.h"

#include <app.h>
#include <render.h>
#include <stdio.h>

#include "scene.h"



LightComponent* LightComponent_add(Entity entity, LightParameters params) {
    LightComponent* light = malloc(sizeof(LightComponent));
    light->disabled = params.disabled;
    light->visibility_mask = params.visibility_mask ? params.visibility_mask : VISIBILITY_NORMAL;
    light->fov = params.fov ? params.fov : 90.0f;
    light->diffuse_color = params.color;
    light->specular_color = params.color;
    light->range = params.range ? params.range : 10.0f;
    light->intensity = params.intensity ? params.intensity : 2.0f;
    light->base_intensity = light->intensity;
    light->flicker_amount = params.flicker_amount;
    light->flicker_speed = params.flicker_speed ? params.flicker_speed : 1.0f;
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
            LOG_ERROR("Unknown light shape: %d", params.shape);
            free(light);
            return NULL;
    }

    scene->components->light[entity] = light;

    return light;
}


void LightComponent_remove(Entity entity) {
    LightComponent* light = scene->components->light[entity];
    if (light) {
        free(light);
        scene->components->light[entity] = NULL;
    }
}
