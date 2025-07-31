#define _USE_MATH_DEFINES

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.h"

#include <systems/navigation.h>
#include <systems/enemy.h>

#include "util.h"
#include "component.h"


Entity create_player(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) { .position = position });
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
    PlayerComponent* player = PlayerComponent_add(i);
    SoundComponent_add(i, (SoundParameters) {});
    WaypointComponent_add(i);

    Entity cam = create_entity();
    TransformComponent_add(cam, (TransformParameters) {
        .position = vec3(0.0f, player->head_height, 0.0f),
    });
    CameraComponent_add(cam,
        (Resolution) { game_settings.width, game_settings.height },
        to_radians(game_settings.fov)
    );
    add_child(i, cam);

    Entity j = create_entity();
    TransformComponent_add(j, (TransformParameters) { .position = vec3(0.0f, -0.5f, 0.1f) });
    look_at(j, vec3(0.0f, -0.5f, -1.0f));
    LightComponent_add(j, (LightParameters) { .shape = LIGHT_SPOT, .color = COLOR_UV, .fov = 50.0f, .visibility_mask = LIGHT_UV });
    add_child(cam, j);

    ArrayList_add(player->inventory, &j);
    player->selected_item = 0;

    return i;
}


Entity create_lamp(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) { .position = position });
    look_at(i, vec3(position.x, position.y - 1.0f, position.z));
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "lamp", .texture_filename = "lamp", .emissive_filename = "lamp" });
    LightComponent_add(i, (LightParameters) { .color = COLOR_WHITE, .visibility_mask = LIGHT_NORMAL });

    return i;
}


Entity create_wall(Vector3 position, float width, float depth, int windows) {
    float wall_height = 1.0f;
    float window_height = 1.5f;

    Entity i = create_entity();
    TransformComponent* trans = TransformComponent_add(i, (TransformParameters) { .position = position });
    trans->position.y = position.y + wall_height * 0.5f;
    trans->scale = vec3(width, wall_height, depth);
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });
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
            trans = TransformComponent_add(window, (TransformParameters) {
                .position = vec3(x, position.y + wall_height + 0.5f * window_height, position.z)
            });
            trans->scale = vec3(segment_width, window_height, depth);
        } else {
            float z = position.z - 0.5f * depth + 0.5f * segment_depth + j * (segment_depth + window_width);
            trans = TransformComponent_add(window, (TransformParameters) {
                .position = vec3(position.x, position.y + wall_height + 0.5f * window_height, z)
            });
            trans->scale = vec3(width, window_height, segment_depth);
        }
        MeshComponent_add(window, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });
        ColliderComponent_add(window, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });
    }

    i = create_entity();
    trans = TransformComponent_add(i, (TransformParameters) { .position = position });
    trans->position.y = position.y + wall_height * 1.5f + window_height;
    trans->scale = vec3(width, 1.0f, depth);
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    return i;
}


Entity create_wall_with_door(Vector3 position, float width, float depth, float door_width) {
    float wall_height = 3.0f;
    float door_height = 2.5f;

    float wall_width = (width > depth) ? (width - door_width) * 0.5f : width;
    float wall_depth = (width > depth) ? depth : (depth - door_width) * 0.5f;

    float x_offset = (width > depth) ? 0.5f * wall_width + 0.5f * door_width : 0.0f;
    float z_offset = (width > depth) ? 0.0f : 0.5f * wall_depth + 0.5f * door_width;

    float door_x_scale = (width > depth) ? door_width : wall_width;
    float door_z_scale = (width > depth) ? wall_depth : door_width;

    Entity left_wall = create_entity();
    TransformComponent_add(left_wall, (TransformParameters) {
        .position = vec3(position.x - x_offset, position.y + wall_height * 0.5f, position.z - z_offset),
        .scale = vec3(wall_width, wall_height, wall_depth)
    });
    MeshComponent_add(left_wall, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "tiles",
        .material_filename = "glass"
    });
    ColliderComponent_add(left_wall, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    Entity right_wall = create_entity();
    TransformComponent_add(right_wall, (TransformParameters) {
        .position = vec3(position.x + x_offset, position.y + wall_height * 0.5f, position.z + z_offset),
        .scale = vec3(wall_width, wall_height, wall_depth)
    });
    MeshComponent_add(right_wall, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "tiles",
        .material_filename = "glass"
    });
    ColliderComponent_add(right_wall, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    Entity door_top = create_entity();
    TransformComponent_add(door_top, (TransformParameters) {
        .position = vec3(position.x, position.y + wall_height - 0.5f * (wall_height - door_height), position.z),
        .scale = vec3(door_x_scale, wall_height - door_height, door_z_scale)
    });
    MeshComponent_add(door_top, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "tiles",
        .material_filename = "glass"
    });
    ColliderComponent_add(door_top, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    return door_top;
}


Quaternion random_y_rotation() {
    return axis_angle_to_quaternion(vec3(0.0f, 1.0f, 0.0f), randf(0.0f, 2.0f * M_PI));
}


Quaternion random_z_rotation() {
    return axis_angle_to_quaternion(vec3(0.0f, 0.0f, 1.0f), randf(0.0f, 2.0f * M_PI));
}


void create_tree(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
        .rotation = random_y_rotation(),
        .scale = diag3(randf(0.5f, 1.0f))
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "tree",
        .texture_filename = "bark",
        .material_filename = "concrete",
    });
}


void create_forest(Vector3 position, float width, float depth, float density, float min_distance) {
    for (float x = -width / 2.0f; x < width / 2.0f; x += density) {
        for (float z = -depth / 2.0f; z < depth / 2.0f; z += density) {
            if (fabs(x) < min_distance && fabs(z) < min_distance) {
                continue; // Skip the center area
            }

            if (rand() % 100 < 10) {
                create_tree(vec3(position.x + x, position.y, position.z + z));
            }
        }
    }
}


Entity create_blood(Vector3 position, bool hidden) {
    Entity i = create_entity();
    Quaternion rotate_x = axis_angle_to_quaternion(vec3(1.0f, 0.0f, 0.0f), to_radians(-90.0f));
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + 1e-3f, position.z),
        .rotation = quaternion_mult(random_y_rotation(), rotate_x),
        .scale = diag3(randf(1.0f, 2.0f))
    });
    MeshParameters params = {
        .mesh_filename = "quad",
        .texture_filename = "blood",
        .material_filename = "glass",
        .visibility = hidden ? LIGHT_UV : LIGHT_ALL
    };
    if (hidden) {
        strcpy(params.material_filename, "hidden");
    }
    MeshComponent_add(i, params);

    if (hidden) {
        return i;
    }

    Entity j = create_entity();
    TransformComponent_add(j, (TransformParameters) {
        .position = vec3(0.0f, 0.0f, 1e-3f),
        .parent = i
    });
    MeshComponent_add(j, (MeshParameters) {
        .mesh_filename = "quad",
        .texture_filename = "blood",
        .material_filename = "hidden",
        .visibility = LIGHT_UV
    });

    return i;
}


void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->screen_camera = create_screen_camera();
    scene->player = create_player(vec3(0.0f, 2.0f, 0.0f));
    TransformComponent* trans = get_component(scene->player, COMPONENT_TRANSFORM);
    scene->camera = trans->children->head->value;

    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, -0.51f, 0.0f),
        .scale = vec3(100.0f, 1.0f, 100.0f)
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "gravel",
        .material_filename = "concrete"
    });

    i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, -0.5f, 0.0f),
        .scale = vec3(10.0f, 1.0f, 10.0f)
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "gravel",
        .material_filename = "concrete"
    });
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    create_forest(zeros3(), 50.0f, 50.0f, 3.0f, 10.0f);

    // create_wall(vec3(0.0f, 0.0f, -5.25f), 10.0f, 0.5f, 3);
    // create_wall(vec3(0.0f, 0.0f, 5.25f), 10.0f, 0.5f, 3);
    // create_wall(vec3(5.25f, 0.0f, 0.0f), 0.5f, 10.0f, 3);
    // create_wall(vec3(-5.25f, 0.0f, 0.0f), 0.5f, 10.0f, 3);

    create_wall_with_door(vec3(0.0f, 0.0f, -2.5f), 5.0f, 0.5f, 1.0f);
    create_wall_with_door(vec3(-2.5f, 0.0f, 0.0f), 0.5f, 5.0f, 1.0f);
    create_wall_with_door(vec3(2.5f, 0.0f, 0.0f), 0.5f, 5.0f, 1.0f);
    create_wall_with_door(vec3(0.0f, 0.0f, 2.5f), 5.0f, 0.5f, 1.0f);

    // create_waypoint(vec3(0.0f, 1.0f, 0.0f));
    // create_waypoint(vec3(2.0f, 1.0f, 2.0f));

    scene->lamp =  create_lamp(vec3(0.0f, 4.0f, 0.0f));

    create_enemy(vec3(2.0f, 0.0f, 2.0f), 0.0f);

    // i = create_entity();
    // TransformComponent_add(i, vec3(0.0f, 2.0f, 0.0f));
    // MeshComponent_add(i, (MeshParameters) {
    //     .mesh_filename = "paper",
    //     .texture_filename = "paper",
    //     .material_filename = "hidden",
    //     .visibility = LIGHT_UV
    // });
    // ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_SPHERE, .group = GROUP_ITEMS });

    // create_blood(vec3(1.0f, 0.5f, 1.0f), false);
    // create_blood(vec3(2.0f, 0.5f, 3.0f), false);
    // create_blood(vec3(-1.0f, 0.5f, -1.0f), true);

    scene->weather = create_entity();
    WeatherComponent_add(scene->weather, (WeatherParameters) {
        .fog_color = COLOR_SKY,
        .fog_start = 10.0f,
        .fog_end = 50.0f
    });

    LOG_INFO("Scene created");
}
