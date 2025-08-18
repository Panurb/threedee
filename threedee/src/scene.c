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
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + 1.0f, position.z),
    });
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
    TransformComponent_add(i, (TransformParameters) { .position = position, .scale = vec3(0.5f, 0.5f, 0.5f) });
    look_at(i, vec3(position.x, position.y - 1.0f, position.z));
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "lamp", .texture_filename = "lamp", .emissive_filename = "lamp" });
    LightComponent_add(i, (LightParameters) { .color = COLOR_WHITE, .visibility_mask = LIGHT_NORMAL });

    return i;
}


Entity create_ground(float width, float depth) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, -0.51f, 0.0f),
        .scale = vec3(width, 1.0f, depth)
    });
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "gravel", .material_filename = "glass" });
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_PLANE, .group = GROUP_WALLS, .height = 0.5f });

    return i;
}


Entity create_floor(Vector3 position, float width, float depth) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y - 0.5f, position.z),
        .scale = vec3(width, 1.0f, depth)
    });
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });

    return i;
}


Entity create_wall_empty(Vector3 position, float width, float depth) {
    float wall_height = 3.5f;

    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + wall_height * 0.5f, position.z),
        .scale = vec3(width, wall_height, depth)
    });
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    return i;
}


void create_frame(Vector3 position, float width, float height, float depth) {
    float thickness = 0.05f;
    float lip = 0.1f;

    MeshParameters mesh_params = {
        .mesh_filename = "cube",
        .texture_filename = "bark",
        .material_filename = "concrete"
    };

    for (int i = 0; i < 2; i++) {
        Entity horizontal = create_entity();
        Vector3 scale;
        if (width > depth) {
            scale = vec3(width, thickness, depth + lip);
        } else {
            scale = vec3(width + lip, thickness, depth);
        }
        TransformComponent_add(horizontal, (TransformParameters) {
            .position = vec3(position.x, position.y + i * (height - thickness) + 0.5f * thickness, position.z),
            .scale = scale
        });
        MeshComponent_add(horizontal, mesh_params);
    }

    for (int i = -1; i < 2; i += 2) {
        Entity vertical = create_entity();
        Vector3 pos;
        Vector3 scale;
        if (width > depth) {
            pos = vec3(position.x + i * 0.5f * (width - thickness), position.y + height * 0.5f, position.z);
            scale = vec3(thickness, height - 2.0f * thickness, depth + lip);
        } else {
            pos = vec3(position.x, position.y + height * 0.5f, position.z + i * 0.5f * (depth - thickness));
            scale = vec3(width + lip, height - 2.0f * thickness, thickness);
        }
        TransformComponent_add(vertical, (TransformParameters) {
            .position = pos,
            .scale = scale
        });
        MeshComponent_add(vertical, mesh_params);
    }
}


Entity create_wall_with_windows(Vector3 position, float width, float depth, int windows) {
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
            TransformComponent_add(window, (TransformParameters) {
                .position = vec3(x, position.y + wall_height + 0.5f * window_height, position.z),
                .scale = vec3(segment_width, window_height, depth)
            });
        } else {
            float z = position.z - 0.5f * depth + 0.5f * segment_depth + j * (segment_depth + window_width);
            TransformComponent_add(window, (TransformParameters) {
                .position = vec3(position.x, position.y + wall_height + 0.5f * window_height, z),
                .scale = vec3(width, window_height, segment_depth)
            });
        }
        MeshComponent_add(window, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "tiles", .material_filename = "glass" });
        ColliderComponent_add(window, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });
    }

    for (int j = 0; j < windows; j++) {
        if (width > depth) {
            create_frame(
                vec3(
                    position.x - 0.5f * width + (segment_width + window_width) * (j + 1) - 0.5f * window_width,
                    position.y + wall_height,
                    position.z
                ),
                window_width,
                window_height,
                depth
            );
        } else {
            create_frame(
                vec3(
                    position.x,
                    position.y + wall_height,
                    position.z - 0.5f * depth + (segment_depth + window_width) * (j + 1) - 0.5f * window_width
                ),
                width,
                window_height,
                window_width
            );
        }

        // Entity window_sill = create_entity();
        // if (width > depth) {
        //     TransformComponent_add(window_sill, (TransformParameters) {
        //         .position = vec3(
        //             position.x - 0.5f * width + (segment_width + window_width) * (j + 1) - 0.5f * window_width,
        //             position.y + wall_height + 0.05f,
        //             position.z
        //         ),
        //         .scale = vec3(window_width + 0.01f, 0.1f, depth + 0.1f)
        //     });
        // } else {
        //     TransformComponent_add(window_sill, (TransformParameters) {
        //       .position = vec3(
        //           position.x,
        //           position.y + wall_height + 0.05f,
        //           position.z - 0.5f * depth + (segment_depth + window_width) * (j + 1) - 0.5f * window_width
        //       ),
        //       .scale = vec3(width + 0.1f, 0.1f, window_width + 0.01f)
        //     });
        // }
        // MeshComponent_add(window_sill, (MeshParameters) {
        //     .mesh_filename = "cube",
        //     .texture_filename = "bark",
        //     .material_filename = "concrete",
        // });
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
    float wall_height = 3.5f;
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

    float wp_x_offset = sign(z_offset);
    float wp_z_offset = sign(x_offset);

    create_frame(
        vec3(position.x, position.y, position.z),
        (width > depth) ? door_width : wall_width,
        door_height,
        (width > depth) ? wall_depth : door_width
    );

    create_waypoint(vec3(position.x - wp_x_offset, position.y, position.z - wp_z_offset));
    create_waypoint(vec3(position.x + wp_x_offset, position.y, position.z + wp_z_offset));

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


Entity create_wall(Vector3 position, float width, float depth, Wall type) {
    switch (type) {
        case WALL_PLAIN:
            return create_wall_empty(position, width, depth);
        case WALL_DOOR:
            return create_wall_with_door(position, width, depth, 1.2f);
        case WALL_WINDOWS:
            return create_wall_with_windows(position, width, depth, 3);
        default:
            return NULL_ENTITY;
    }
}


Coordinates direction_offset(Direction direction) {
    switch (direction) {
        case DIRECTION_FRONT:
            return (Coordinates) { 0, 1 };
        case DIRECTION_RIGHT:
            return (Coordinates) { 1, 0 };
        case DIRECTION_BACK:
            return (Coordinates) { 0, -1 };
        case DIRECTION_LEFT:
            return (Coordinates) { -1, 0 };
    }

    return (Coordinates) { 0, 0 };
}


void set_wall_type(Level* level, int x, int z, Direction direction, Wall wall_type) {
    if (x < 0 || x >= level->width || z < 0 || z >= level->depth) {
        return;
    }

    Room* room = &level->rooms[z][x];
    room->walls[direction] = wall_type;

    Direction opposite_direction = (direction + 2) % 4;
    Coordinates offset = direction_offset(direction);
    Room* adjacent_room = &level->rooms[z + offset.z][x + offset.x];
    adjacent_room->walls[opposite_direction] = wall_type;
}


void print_level(Level level) {
    char wall_types[] = " NPDW";

    for (int i = 0; i < level.depth; i++) {
        for (int j = 0; j < level.width; j++) {
            Room room = level.rooms[i][j];
            printf(" %c |", wall_types[room.walls[DIRECTION_BACK]]);
        }
        printf("\n");

        for (int j = 0; j < level.width; j++) {
            Room room = level.rooms[i][j];
            printf(
                "%c%c%c|",
                wall_types[room.walls[DIRECTION_LEFT]],
                room.lamp ? 'L' : ' ',
                wall_types[room.walls[DIRECTION_RIGHT]]
            );
        }
        printf("\n");

        for (int j = 0; j < level.width; j++) {
            Room room = level.rooms[i][j];
            printf(" %c |", wall_types[room.walls[DIRECTION_FRONT]]);
        }
        printf("\n-----------------\n");
    }
}


void generate_level(Level* level, int x, int z) {
    if (x < 0 || x >= level->width || z < 0 || z >= level->depth) {
        return;
    }

    LOG_INFO("Generating level at (%d, %d)", x, z);

    print_level(*level);

    Room* room = &level->rooms[z][x];

    room->floor = true;
    room->lamp = randf(0.0f, 1.0f) < 0.5f;

    int available_directions[4];
    int available_count = 0;

    for (Direction d = 0; d < 4; d++) {
        Wall wall = room->walls[d];
        if (wall == WALL_UNSET) {
            available_directions[available_count] = d;
            available_count++;
        }
    }

    if (available_count == 0) {
        return;
    }

    permute(available_directions, available_count);

    int blocked_count = randi(1, 2);

    for (int i = 0; i < available_count; i++) {
        Direction direction = available_directions[i];
        Direction opposite_direction = (direction + 2) % 4;
        Coordinates offset = direction_offset(direction);

        bool out_of_bounds = (z + offset.z < 0 || z + offset.z >= level->depth ||
                            x + offset.x < 0 || x + offset.x >= level->width);

        if (out_of_bounds) {
            room->walls[direction] = WALL_WINDOWS;
            continue;
        }

        if (i < blocked_count) {
            set_wall_type(level, x, z, direction, WALL_PLAIN);
            continue;
        }

        Wall wall_type = randf(0.0f, 1.0f) < 0.5f ? WALL_DOOR : WALL_NONE;
        room->walls[direction] = wall_type;
        level->rooms[z + offset.z][x + offset.x].walls[opposite_direction] = wall_type;
        generate_level(level, x + offset.x, z + offset.z);
    }
}


Level create_level() {
    Level level = {
        .width = 5,
        .depth = 5,
        .room_width = 10.0f,
        .room_depth = 10.0f,
    };

    generate_level(&level, level.width / 2, level.depth / 2);

    LOG_INFO("Make outside walls windows");
    for (int i = 0; i < level.width; i++) {
        for (int j = 0; j < level.depth; j++) {
            if (level.rooms[j][i].floor) continue;

            for (Direction d = 0; d < 4; d++) {
                if (level.rooms[j][i].walls[d] == WALL_PLAIN) {
                    if (randf(0.0f, 1.0f) < 0.5f) {
                        set_wall_type(&level, i, j, d, WALL_WINDOWS);
                    }
                }
            }
        }
    }

    print_level(level);

    create_ground(100.0f, 100.0f);

    for (int i = 0; i < level.width; i++) {
        for (int j = 0; j < level.depth; j++) {
            Room room = level.rooms[j][i];

            float x_offset = (i - level.width / 2) * level.room_width;
            float z_offset = (j - level.depth / 2) * level.room_depth;
            Vector3 pos = vec3(x_offset, 0.0f, z_offset);

            if (room.floor) {
                create_floor(pos, level.room_width, level.room_depth);
                create_floor(vec3(pos.x, 4.0f, pos.z), level.room_width, level.room_depth);
                create_waypoint(pos);
            }

            if (room.lamp) {
                create_lamp(vec3(pos.x, 2.5f, pos.z));
            }

            // Only create back-left walls for the first row and column
            Direction max_direction = (i == 0 || j == 0) ? 4 : 2;

            for (Direction d = 0; d < max_direction; d++) {
                Coordinates offset = direction_offset(d);

                Vector3 wall_pos = vec3(
                    pos.x + offset.x * 0.5f * level.room_width,
                    pos.y,
                    pos.z + offset.z * 0.5f * level.room_depth
                );

                float width = (d == DIRECTION_FRONT || d == DIRECTION_BACK) ? level.room_width : 0.5f;
                float depth = (d == DIRECTION_FRONT || d == DIRECTION_BACK) ? 0.5f : level.room_width;

                create_wall(wall_pos, width, depth, room.walls[d]);
            }
        }
    }

    return level;
}


void create_scene() {
    LOG_INFO("Creating scene");

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->screen_camera = create_screen_camera();
    scene->player = create_player(zeros3());
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

    create_forest(zeros3(), 100.0f, 100.0f, 2.0f, 30.0f);

    Level level = create_level();

    while (true) {
        int x = randi(0, level.width - 1);
        int z = randi(0, level.depth - 1);

        if (x == 0 && z == 0) {
            continue; // Skip the starting room
        }

        if (level.rooms[z][x].floor) {
            create_enemy(vec3(
                (x - level.width / 2) * level.room_width,
                0.0f,
                (z - level.depth / 2) * level.room_depth
            ), randf(0.0f, 360.0f));
            break;
        }
    }

    scene->weather = create_entity();
    WeatherComponent_add(scene->weather, (WeatherParameters) {
        .fog_color = COLOR_SKY,
        .fog_start = 10.0f,
        .fog_end = 50.0f,
        .ambient_light = 0.05f,
    });

    LOG_INFO("Scene created");
}
