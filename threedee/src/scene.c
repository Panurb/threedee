#include "scene.h"

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "component.h"


void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->camera = create_camera();
    scene->menu_camera = create_menu_camera();

    LOG_INFO("Scene created");
}
