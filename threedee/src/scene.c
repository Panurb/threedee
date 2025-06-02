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

    scene->player = create_entity();
    TransformComponent_add(scene->player, zeros3());
    // add_child(scene->player, scene->camera);

    Entity i = create_entity();
    TransformComponent_add(i, (Vector3){1.0f, 0.0f, 0.0f});
    MeshComponent_add(i, "cube_textured");

    LOG_INFO("Scene created");
}
