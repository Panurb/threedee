#include <stdio.h>

#include "scene.h"


void update_lights(float time_step) {
    UNUSED(time_step);

    for (Entity i = 0; i < scene->components->entities; i++) {
        LightComponent* light = get_component(i, COMPONENT_LIGHT);
        if (!light) continue;

        if (light->flicker_amount > 0.0f) {
            float min_intensity = light->base_intensity * (1.0f - light->flicker_amount);

            light->intensity -= light->flicker_speed * light->base_intensity * time_step;
            if (light->intensity < min_intensity) {
                light->intensity = randf(0.9f, 1.0f) * light->base_intensity;
            }
        }
    }
}
