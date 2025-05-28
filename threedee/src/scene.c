#include "scene.h"

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "component.h"


void create_scene() {
    LOG_INFO("Creating scene");

    game_data = malloc(sizeof(Scene));
    game_data->components = ComponentData_create();
    game_data->camera = create_camera();
    game_data->menu_camera = create_menu_camera();

    LOG_INFO("Scene created");
}
