#pragma once

#include "component.h"


typedef struct Scene {
    Entity menu_camera;
    ComponentData* components;
} Scene;


Scene* game_data;


void create_scene();
