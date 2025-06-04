#define _USE_MATH_DEFINES

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "scene.h"

#include "util.h"
#include "component.h")


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
    TransformComponent* trans = TransformComponent_add(i, (Vector3){0.0f, -2.0f, 0.0f});
    trans->scale.x = 10.0f;
    trans->scale.z = 10.0f;
    MeshComponent_add(i, "cube_textured", "gravel");

    i = create_entity();
    trans = TransformComponent_add(i, (Vector3){5.5f, 0.0f, 0.0f});
    trans->scale.y = 3.0f;
    trans->scale.z = 10.0f;
    MeshComponent_add(i, "cube_textured", "brick_tile");

    i = create_entity();
    trans = TransformComponent_add(i, (Vector3){0.0f, 0.0f, 5.5f});
    trans->scale.y = 3.0f;
    trans->scale.x = 10.0f;
    MeshComponent_add(i, "cube_textured", "brick_tile");

    i = create_entity();
    trans = TransformComponent_add(i, (Vector3){-5.5f, 0.0f, 0.0f});
    trans->scale.y = 3.0f;
    trans->scale.z = 10.0f;
    MeshComponent_add(i, "cube_textured", "brick_tile");

    i = create_entity();
    trans = TransformComponent_add(i, (Vector3){0.0f, 0.0f, -5.5f});
    trans->scale.y = 3.0f;
    trans->scale.x = 10.0f;
    MeshComponent_add(i, "cube_textured", "brick_tile");

    i = create_entity();
    TransformComponent_add(i, (Vector3){-3.0f, 0.0f, 0.0f});
    MeshComponent_add(i, "sphere", "gravel");

    LOG_INFO("Scene created");
}
