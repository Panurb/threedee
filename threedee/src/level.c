#include <stdio.h>

#include "level.h"
#include "prefab.h"
#include "systems/navigation.h"
#include "perlin.h"


Vector3 wall_direction(Direction direction) {
    switch (direction) {
        case DIRECTION_FRONT:
            return vec3(0.0f, 0.0f, 1.0f);
        case DIRECTION_RIGHT:
            return vec3(1.0f, 0.0f, 0.0f);
        case DIRECTION_BACK:
            return vec3(0.0f, 0.0f, -1.0f);
        case DIRECTION_LEFT:
            return vec3(-1.0f, 0.0f, 0.0f);
        default:
            return zeros3();
    }
}


float wall_angle(Direction direction) {
    switch (direction) {
        case DIRECTION_FRONT:
            return 0.0f;
        case DIRECTION_RIGHT:
            return -90.0f;
        case DIRECTION_BACK:
            return 180.0f;
        case DIRECTION_LEFT:
            return 90.0f;
        default:
            return 0.0f;
    }
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


Entity create_ceiling(Vector3 position, float width, float depth) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y - 0.5f, position.z),
        .scale = vec3(width, 1.0f, depth)
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "cube",
        .material_filename = "glass",
        .texture_filename = "plaster"
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_AABB,
        .group = GROUP_WALLS
    });

    return i;
}


Entity create_floor(Vector3 position, float width, float depth, String texture_filename) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y - 0.5f, position.z),
        .scale = vec3(width, 1.0f, depth)
    });
    MeshParameters params = {
        .mesh_filename = "cube",
        .material_filename = "glass"
    };
    strcpy(params.texture_filename, texture_filename);
    MeshComponent_add(i, params);

    return i;
}


Entity create_wall_empty(Vector3 position, float width, float depth) {
    float wall_height = 3.5f;

    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + wall_height * 0.5f, position.z),
        .scale = vec3(width, wall_height, depth)
    });
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "plaster", .material_filename = "glass" });
    ColliderComponent_add(i, (ColliderParameters) { .type = COLLIDER_AABB, .group = GROUP_WALLS });

    return i;
}


void create_frame(Vector3 position, float width, float height, float depth) {
    float thickness = 0.05f;
    float lip = 0.1f;

    MeshParameters mesh_params = {
        .mesh_filename = "cube",
        .texture_filename = "wood",
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
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "plaster", .material_filename = "glass" });
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
        MeshComponent_add(window, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "plaster", .material_filename = "glass" });
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
    }

    i = create_entity();
    trans = TransformComponent_add(i, (TransformParameters) { .position = position });
    trans->position.y = position.y + wall_height * 1.5f + window_height;
    trans->scale = vec3(width, 1.0f, depth);
    MeshComponent_add(i, (MeshParameters) { .mesh_filename = "cube", .texture_filename = "plaster", .material_filename = "glass" });
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
        .texture_filename = "plaster",
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
        .texture_filename = "plaster",
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
        .texture_filename = "plaster",
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


void create_forest(float width, float depth, float density, Level level) {
    float frequency = 0.4f;
    float distance = 1.0f / density;

    Permutation permutation;
    init_perlin(permutation);

    float origin_x = 0.5f * level.width * level.room_width;
    float origin_z = 0.5f * level.depth * level.room_depth;

    for (float x = -width / 2.0f; x < width / 2.0f; x += distance) {
        for (float z = -depth / 2.0f; z < depth / 2.0f; z += distance) {
            int i = (int)floorf((x + origin_x) / level.room_width);
            int j = (int)floorf((z + origin_z) / level.room_depth);
            if (0 <= i && i < level.width
                && 0 <= j && j < level.depth
                && level.rooms[j][i].type != ROOM_EMPTY
            ) {
                continue;
            }

            float prob = perlin(x * frequency, 0.0f, z * frequency, permutation, 0);

            float offset_x = sign(x) * randf(0.0f, distance);
            float offset_z = sign(z) * randf(0.0f, distance);

            if (prob < 0.3f) {
                create_tree(vec3(x + offset_x, 0.0f, z + offset_z));
            } else if (prob < 0.4f) {
                create_shrub(vec3(x + offset_x, 0.0f, z + offset_z));
            }
        }
    }
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
                "%c %c|",
                wall_types[room.walls[DIRECTION_LEFT]],
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

    LOG_DEBUG("Generating level at (%d, %d)", x, z);

    if (LOGGING_LEVEL > 3) {
        print_level(*level);
    }

    Room* room = &level->rooms[z][x];

    RoomType room_types[] = { ROOM_BATHROOM, ROOM_HALLWAY, ROOM_BEDROOM, ROOM_LIVINGROOM, ROOM_KITCHEN };
    float probs[] = { 0.1f, 0.4f, 0.25f, 0.25f, 0.2f };

    if (x == level->width / 2 && z == level->depth / 2) {
        probs[4] = 0.0f;
    }

    room->type = room_types[rand_choice(probs, LENGTH(probs))];
    room->floor = true;

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
        .room_height = 3.0f
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

    create_fire(vec3(0.0f, 1.0f, 0.0f), 0.5f);

    create_blood_dripper(vec3(2.0f, level.room_height, 2.0f));
    create_blood(vec3(2.0f, 0.0f, 2.0f), false);

    for (int i = 0; i < level.width; i++) {
        for (int j = 0; j < level.depth; j++) {
            Room room = level.rooms[j][i];

            float x_offset = (i - level.width / 2) * level.room_width;
            float z_offset = (j - level.depth / 2) * level.room_depth;
            Vector3 pos = vec3(x_offset, 0.0f, z_offset);

            if (room.floor) {
                create_dust_particles(
                    pos,
                    level.room_width,
                    level.room_depth,
                    4.0f
                );

                create_floor(pos, level.room_width, level.room_depth, "tiles");
                create_ceiling(vec3(pos.x, 4.0f, pos.z), level.room_width, level.room_depth);
                create_waypoint(pos);

                if (chance(0.25f)) {
                    float dx = randf(-0.5f * level.room_width + 1.0f, 0.5f * level.room_width - 1.0f);
                    float dz = randf(-0.5f * level.room_depth + 1.0f, 0.5f * level.room_depth - 1.0f);
                    create_blood(vec3(pos.x + dx, pos.y, pos.z + dz), false);
                }
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

                float width = (d == DIRECTION_FRONT || d == DIRECTION_BACK) ? level.room_width - 0.5f : 0.5f;
                float depth = (d == DIRECTION_FRONT || d == DIRECTION_BACK) ? 0.5f : level.room_width - 0.5f;

                create_wall(wall_pos, width, depth, room.walls[d]);

                Vector3 corner = vec3(
                    pos.x + offset.x * 0.5f * level.room_width - offset.z * 0.5f * level.room_width,
                    pos.y,
                    pos.z + offset.z * 0.5f * level.room_depth - offset.x * 0.5f * level.room_depth
                );
                if (room.walls[d] != WALL_NONE && room.walls[d] != WALL_UNSET) {
                    create_wall_empty(corner, 0.5f, 0.5f);
                }
            }

            switch (room.type) {
                case ROOM_BATHROOM:
                    break;
                case ROOM_HALLWAY:
                    break;
                case ROOM_BEDROOM:
                    create_lamp(vec3(pos.x, 2.5f, pos.z));
                    break;
                case ROOM_LIVINGROOM:
                    for (int w = 0; w < 3; w++) {
                        Wall wall = room.walls[w];
                        Vector3 dir = wall_direction(w);
                        if (wall == WALL_PLAIN) {
                            create_television(
                                vec3(
                                    pos.x + dir.x * (0.5f * level.room_width - 0.5f),
                                    pos.y + 0.5f,
                                    pos.z + dir.z * (0.5f * level.room_depth - 0.5f)
                                ),
                                -wall_angle(w)
                            );
                            break;
                        }
                    }
                    create_lamp(vec3(pos.x, 2.5f, pos.z));
                    break;
                case ROOM_KITCHEN:
                    create_lamp(vec3(pos.x, 2.5f, pos.z));
                    create_table(pos, 0.0f);
                    for (int c = 0; c < 4; c++) {
                        float angle = to_radians(c * 90.0f);
                        float d = (c % 2 == 0) ? 1.5f : 1.0f;
                        create_chair(
                            add3(pos, vec3(cosf(angle) * d, 0.0f, sinf(angle) * d)),
                            -(c + 1) * 90.0f
                        );
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return level;
}
