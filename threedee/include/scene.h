#pragma once

#include "component.h"


typedef struct Scene {
    Entity camera;
    Entity menu_camera;
    Entity player;
    ComponentData* components;
} Scene;


Scene* scene;


void create_scene();
