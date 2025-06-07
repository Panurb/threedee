#pragma once

#include "component.h"


typedef struct {
    Color diffuse_color;
    Color specular_color;
    float range;
    float intensity;
} LightComponent;


void LightComponent_add(int entity, Color color);


void LightComponent_remove(int entity);
