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
    rb->friction = 0.0f;
    rb->can_sleep = false;
    // MeshComponent_add(i, "cube", "tiles", "default");
    ColliderComponent_add(i,
        (ColliderParameters) {
            .type = COLLIDER_CAPSULE,
            .group = GROUP_PLAYERS,
            .radius = 0.4f,
            .height = 1.0f
        }
    );
    ControllerComponent_add(i, -1);
    Entity cam = create_camera();
    add_child(i, cam);

    Entity j = create_entity();
    TransformComponent_add(j, vec3(0.0f, 0.5f, 0.1f));
    look_at(j, vec3(0.0f, 0.5f, -1.0f));
    LightComponent_add(j, (LightParameters) { .type = LIGHT_SPOT, .color = COLOR_WHITE, .fov = 40.0f });
    add_child(cam, j);

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


Entity create_wall(Vector3 position, float width, float depth, int windows) {
    Entity i = create_entity();
    TransformComponent* trans = TransformComponent_add(i, position);
    trans->position.y = position.y - 1.0f;
    trans->scale = vec3(width, 1.0f, depth);
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    float window_width = 1.0f;
    float wall_width = width - windows * window_width;
    float wall_depth = depth - windows * window_width;
    float segment_width = wall_width / (float) (windows + 1);
    float segment_depth = wall_depth / (float) (windows + 1);

    for (int j = 0; j < windows + 1; j++) {
        Entity window = create_entity();
        if (width > depth) {
            float x = position.x - 0.5f * width + 0.5f * segment_width + j * (segment_width + window_width);
            trans = TransformComponent_add(window, vec3(x, position.y, position.z));
            trans->scale = vec3(segment_width, 1.5f, depth);
        } else {
            float z = position.z - 0.5f * depth + 0.5f * segment_depth + j * (segment_depth + window_width);
            trans = TransformComponent_add(window, vec3(position.x, position.y, z));
            trans->scale = vec3(width, 1.5f, segment_depth);
        }
        MeshComponent_add(window, "cube", "tiles", "glass");
        ColliderComponent_add(window, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });
    }

    i = create_entity();
    trans = TransformComponent_add(i, position);
    trans->position.y = position.y + 1.0f;
    trans->scale = vec3(width, 0.5f, depth);
    MeshComponent_add(i, "cube", "tiles", "glass");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    return i;
}



void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->menu_camera = create_menu_camera();
    scene->ambient_light = 0.2f;
    scene->player = create_player(vec3(0.0f, 2.0f, 0.0f));
    TransformComponent* trans = get_component(scene->player, COMPONENT_TRANSFORM);
    scene->camera = trans->children->head->value;

    Entity i = create_entity();
    trans = TransformComponent_add(i, vec3(0.0f, -2.0f, 0.0f));
    trans->scale.x = 10.0f;
    trans->scale.z = 10.0f;
    MeshComponent_add(i, "cube", "gravel", "concrete");
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    create_wall(vec3(0.0f, 0.0f, -5.25f), 10.0f, 0.5f, 3);
    create_wall(vec3(0.0f, 0.0f, 5.25f), 10.0f, 0.5f, 3);
    create_wall(vec3(5.25f, 0.0f, 0.0f), 0.5f, 10.0f, 3);
    create_wall(vec3(-5.25f, 0.0f, 0.0f), 0.5f, 10.0f, 3);

    for (int j = 0; j < 5; j++) {
        i = create_entity();
        TransformComponent_add(i, vec3((float) j, 1.5f, -2.0f));
        MeshComponent_add(i, "cube", "tiles", "default");
        RigidBodyComponent* rb = RigidBodyComponent_add(i, 1.0f);
        // rb->axis_lock.rotation = true;
        ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_CUBOID, .group = GROUP_PROPS, .width = 1.0f, .height = 1.0f, .depth = 1.0f });
    }

    for (int j = 0; j < 5; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3((float) j, 1.0f, 0.0f));
        transform->scale = vec3(0.5f, 0.5f, 0.5f);
        MeshComponent_add(i, "sphere", "bark", "default");
        RigidBodyComponent* rigid_body = RigidBodyComponent_add(i, 1.0f);
        // rigid_body->angular_velocity = vec3(0.0f, 1.0f, 0.0f);
        ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_SPHERE, .group = GROUP_PROPS, .radius = 1.0f });
    }

    for (int j = 0; j < 1; j++) {
        i = create_entity();
        TransformComponent* transform = TransformComponent_add(i, vec3((float) j, 1.0f, 2.0f));
        MeshComponent_add(i, "cube", "bark", "default");
        RigidBodyComponent_add(i, 1.0f);
        ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_CAPSULE, .group = GROUP_PROPS, .radius = 0.5f, .height = 1.0f });
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
