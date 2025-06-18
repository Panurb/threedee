#define _USE_MATH_DEFINES

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.h"
#include "util.h"
#include "component.h"


Entity create_player(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, position);
    RigidBodyComponent* rb = RigidBodyComponent_add(i, 1.0f);
    rb->axis_lock.rotation = true;
    rb->bounce = 0.0f;
    // MeshComponent_add(i, "cube", "tiles", "default");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_CAPSULE, .radius = 0.4f, .height = 1.0f });
    ControllerComponent_add(i, -1);
    add_child(i, scene->camera);

    Entity j = create_entity();
    TransformComponent* trans = TransformComponent_add(j, vec3(0.0f, 1.0f, 1.0f));
    look_at(j, vec3(0.0f, 0.0f, -1.0f));
    LightComponent_add(j, (LightParameters) { .color = COLOR_WHITE, .fov = 30.0f });
    // add_child(scene->camera, j);

    return i;
}


Entity create_lamp(Vector3 position) {
    Entity i = create_entity();
    TransformComponent* trans = TransformComponent_add(i, position);
    trans->scale = vec3(1.0f, 1.0f, 1.0f);
    RigidBodyComponent_add(i, 1.0f);
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_CUBOID, .width = 1.0f, .height = 1.0f, .depth = 1.0f });
    MeshComponent_add(i, "cube", "bark", "default");

    Entity j = create_entity();
    TransformComponent_add(j, vec3(0.0f, 1.0f, 0.0f));
    LightComponent* light = LightComponent_add(j, (LightParameters) { .color = COLOR_YELLOW });
    add_child(i, j);

    return i;
}


void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->camera = create_camera();
    scene->menu_camera = create_menu_camera();
    scene->ambient_light = 0.2f;
    scene->player = create_player(vec3(0.0f, 1.0f, 0.0f));

    Entity i = create_entity();
    TransformComponent* trans = TransformComponent_add(i, vec3(0.0f, -2.0f, 0.0f));
    trans->scale.x = 100.0f;
    trans->scale.z = 100.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube", "gravel", "concrete");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .height = 0.5f });

    i = create_entity();
    trans = TransformComponent_add(i, vec3(5.5f, 0.0f, 0.0f));
    trans->scale.x = 3.0f;
    trans->scale.z = 10.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 90.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .height = 0.5f });

    i = create_entity();
    trans = TransformComponent_add(i, vec3(0.0f, 0.0f, 5.5f));
    trans->scale.x = 10.0f;
    trans->scale.z = 3.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, -90.0f });
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .height = 0.5f });

    i = create_entity();
    trans = TransformComponent_add(i, vec3(-5.5f, 0.0f, 0.0f));
    trans->scale.x = 3.0f;
    trans->scale.z = 10.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { -90.0f, 0.0f, 0.0f });
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .height = 0.5f });

    i = create_entity();
    trans = TransformComponent_add(i, vec3(0.0f, 0.0f, -5.5f));
    trans->scale.x = 10.0f;
    trans->scale.z = 3.0f;
    trans->rotation = euler_to_quaternion((EulerAngles) { 0.0f, 0.0f, 90.0f });
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .height = 0.5f });

    for (int j = 0; j < 0; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3(0.0f, -1.5f, 0.0f));
        // transform->rotation = euler_to_quaternion((EulerAngles) { 10.0f, 5 * j, 0.0f });
        transform->scale = vec3(0.5f, 1.0f, 0.5f);
        MeshComponent_add(i, "cube", "tiles", "default");
        RigidBodyComponent* rb = RigidBodyComponent_add(i, 1.0f);
        rb->axis_lock.rotation = true;
        ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_CAPSULE, .radius = 0.25f, .height = 1.0f });
    }

    for (int j = 0; j < 5; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3((float) j, -1.0f, 0.0f));
        transform->scale = vec3(0.5f, 0.5f, 0.5f);
        MeshComponent_add(i, "sphere", "bark", "default");
        RigidBodyComponent* rigid_body = RigidBodyComponent_add(i, 1.0f);
        // rigid_body->angular_velocity = vec3(0.0f, 1.0f, 0.0f);
        ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_SPHERE, .radius = 1.0f });
    }

    // i = create_entity();
    // TransformComponent_add(i, vec3(5.0f, 5.0f, 0.0f));
    // look_at(i, zeros3());
    // LightComponent_add(i, (LightParameters) { .color = COLOR_BLUE });
    //
    // i = create_entity();
    // TransformComponent_add(i, vec3(-5.0f, 5.0f, 0.0f));
    // look_at(i, zeros3());
    // LightComponent_add(i, (LightParameters) { .color = COLOR_RED });

    // create_lamp(vec3(0.0f, 2.0f, 0.0f));

    LOG_INFO("Scene created");
}
