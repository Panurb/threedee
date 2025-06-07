#include <stdlib.h>

#include "component.h"
#include "components/light.h"
#include "scene.h"



void LightComponent_add(Entity entity, Color color) {
    LightComponent* light = malloc(sizeof(LightComponent));
    light->diffuse_color = color;
    light->specular_color = color;
    light->range = 20.0f;
    light->intensity = 1.0f;

    scene->components->light[entity] = light;
}


void LightComponent_remove(Entity entity) {
    LightComponent* light = scene->components->light[entity];
    if (light) {
        free(light);
        scene->components->light[entity] = NULL;
    }
}
