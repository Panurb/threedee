#define _USE_MATH_DEFINES

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.h"
#include "util.h"
#include "component.h"


void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->camera = create_camera();
    scene->menu_camera = create_menu_camera();
    scene->ambient_light = 0.2f;

    scene->player = create_entity();
    TransformComponent_add(scene->player, zeros3());
    // ColliderComponent_add(scene->camera, COLLIDER_SPHERE, 0.5f);
    // add_child(scene->player, scene->camera);

    Entity i = create_entity();
    TransformComponent* trans = TransformComponent_add(i, vec3(0.0f, -2.0f, 0.0f));
    trans->scale.x = 10.0f;
    trans->scale.z = 10.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube_textured", "gravel", "concrete");
    ColliderComponent_add(i, COLLIDER_PLANE, 0.5f);

    i = create_entity();
    trans = TransformComponent_add(i, vec3(5.5f, 0.0f, 0.0f));
    trans->scale.x = 3.0f;
    trans->scale.z = 10.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 90.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube_textured", "tiles", "glass");
    ColliderComponent_add(i, COLLIDER_PLANE, 0.5f);

    i = create_entity();
    trans = TransformComponent_add(i, vec3(0.0f, 0.0f, 5.5f));
    trans->scale.x = 10.0f;
    trans->scale.z = 3.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, -90.0f });
    MeshComponent_add(i, "cube_textured", "tiles", "glass");
    ColliderComponent_add(i, COLLIDER_PLANE, 0.5f);

    i = create_entity();
    trans = TransformComponent_add(i, vec3(-5.5f, 0.0f, 0.0f));
    trans->scale.x = 3.0f;
    trans->scale.z = 10.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { -90.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube_textured", "tiles", "glass");
    ColliderComponent_add(i, COLLIDER_PLANE, 0.5f);

    i = create_entity();
    trans = TransformComponent_add(i, vec3(0.0f, 0.0f, -5.5f));
    trans->scale.x = 10.0f;
    trans->scale.z = 3.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, 90.0f });
    MeshComponent_add(i, "cube_textured", "tiles", "glass");
    ColliderComponent_add(i, COLLIDER_PLANE, 0.5f);

    for (int j = 0; j < 1; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3((float) j, 1.0f, 0.0f));
        transform->rotation = euler_to_quaternion((EulerAngles) { 10.0f, 0.0f, 0.0f });
        MeshComponent_add(i, "cube_textured", "tiles", "default");
        RigidBodyComponent_add(i, 1.0f);
        ColliderComponent_add(i, COLLIDER_CUBE,  0.5f);
    }

    for (int j = 0; j < 1; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3((float) j, -1.0f, 0.0f));
        transform->scale = vec3(0.5f, 0.5f, 0.5f);
        MeshComponent_add(i, "sphere", "tiles", "default");
        RigidBodyComponent* rigid_body = RigidBodyComponent_add(i, 1.0f);
        // rigid_body->angular_velocity = vec3(0.0f, 1.0f, 0.0f);
        ColliderComponent_add(i, COLLIDER_SPHERE,  1.0f);
    }

    i = create_entity();
    TransformComponent_add(i, vec3(5.0f, 10.0f, 0.0f));
    LightComponent_add(i, COLOR_BLUE);

    i = create_entity();
    TransformComponent_add(i, vec3(-5.0f, 10.0f, 0.0f));
    LightComponent_add(i, COLOR_RED);

    LOG_INFO("Scene created");
}
