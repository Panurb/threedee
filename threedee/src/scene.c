#include "scene.h"

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "component.h"


void create_scene() {
    LOG_INFO("Creating scene");

    Scene* scene = malloc(sizeof(Scene));
    scene->menu_camera = NULL_ENTITY;
    scene->components = ComponentData_create();
    game_data = scene;

    LOG_INFO("Scene created");
}
