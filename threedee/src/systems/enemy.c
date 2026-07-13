#include <stdio.h>

#include "systems/enemy.h"
#include "components/enemy.h"
#include "render.h"
#include "scene.h"
#include "util.h"
#include "raycast.h"
#include "systems/navigation.h"
#include "systems/player.h"


void enemy_seen(Entity trigger, Entity player) {
    UNUSED(trigger);

    EnemyComponent* enemy = get_component(trigger, COMPONENT_ENEMY);
    if (!enemy) return;

    if (enemy->state == ENEMY_STARE) {
        return;
    }

    enemy->state = ENEMY_STARE;
    enemy->target = player;

    player_look(trigger, player);
}


Entity create_enemy(Vector3 pos, float yaw) {
    Entity i = create_entity();

    TransformComponent_add(i, (TransformParameters) {
        .position = add3(pos, vec3(0.0f, 1.0f, 0.0f)),
        .yaw = yaw
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_CAPSULE,
        .radius = 0.5f,
        .height = 1.5f,
        .group = GROUP_ENEMIES
    });

    RigidBodyComponent* rb = RigidBodyComponent_add(i, (RigidBodyParameters) {
        .mass = 80.0f,
        .friction = 1.0f,
        .bounce = 0.0f,
        .axis_lock.rotation = true,
        .dont_sleep = true
    });
    rb->linear_damping = 0.99f;
    EnemyComponent_add(i);
    WaypointComponent_add(i);
    SoundComponent_add(i, (SoundParameters) {});

    Entity mesh = create_entity();
    TransformComponent_add(mesh, (TransformParameters) {
        .position = vec3(0.0f, -(0.5f + 0.75f), 0.0f),
        .parent = i
    });
    MeshComponent_add(mesh, (MeshParameters) {
        .mesh_filename = "enemy",
        .texture_filename = "black",
        .material_filename = "default",
        .visibility = VISIBILITY_ALL
    });
    TriggerComponent_add(i, (TriggerParameters) {
            .type = TRIGGER_LOOK,
            .trigger_group = GROUP_PLAYERS,
            .distance = 20.0f,
            .roi = 1.0f,
            .on_enter = enemy_seen
    });

    return i;
}


void update_vision(int entity) {
    EnemyComponent* enemy = get_component(entity, COMPONENT_ENEMY);

    Vector3 pos = get_position(entity);

    Vector3 r = sub3(get_position(scene->player), pos);
    float angle = dot3(normalized3(r), quaternion_forward(get_rotation(entity)));

    float d = norm3(r);
    if (d < enemy->vision_range && angle > cosf(0.5f * enemy->fov)) {
        Hit info = raycast((Ray) { pos, r, enemy->vision_range }, GROUP_PLAYERS | GROUP_WALLS);
        if (info.entity == scene->player) {
            enemy->target = scene->player;
            enemy->state = ENEMY_CHASE;
        }
    }
}


void follow_target(Entity i, float target_speed) {
    EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
    RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);
    Vector3 pos = get_position(i);

    a_star(i, enemy->target, enemy->path);

    if (enemy->path->size > 1) {
        enemy->desired_direction = get_position(enemy->path->head->next->value);
    } else {
        enemy->desired_direction = get_position(enemy->target);
    }

    Vector3 r = sub3(enemy->desired_direction, pos);

    float speed = norm3(rb->velocity);
    if (speed < target_speed) {
        rb->acceleration = add3(rb->acceleration, mul3(enemy->acceleration, normalized3(r)));
    }
}


void update_enemies(float time_step) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
        if (!enemy) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);
        Vector3 pos = get_position(i);

        // if (enemy->state != ENEMY_ATTACK && enemy->state != ENEMY_DEAD) {
        //     update_vision(i);
        //     Vector3 dir = vec3(enemy->desired_direction.x, pos.y, enemy->desired_direction.z);
        //     turn_to(i, dir, enemy->turn_speed, time_step);
        // }

        switch (enemy->state) {
            case ENEMY_IDLE:
                enemy->desired_direction = quaternion_forward(get_rotation(i));
                break;
            case ENEMY_WANDER:
                break;
            case ENEMY_INVESTIGATE:
                follow_target(i, enemy->walk_speed);
                break;
            case ENEMY_CHASE:
                follow_target(i, enemy->run_speed);
                break;
            case ENEMY_ATTACK:
                if (enemy->attack_timer <= 0.0f) {
                    // attack(enemy->weapon);
                    enemy->state = ENEMY_CHASE;
                } else {
                    enemy->attack_timer -= time_step;
                }

                break;
            case ENEMY_STARE:
                if (!in_player_view(scene->player, i, INFINITY, 1.0f)) {
                    destroy_entity_recursive(i);
                    scene->enemy = NULL_ENTITY;
                }
                break;
            case ENEMY_DEAD:
                break;
        }
    }
}


void debug_draw_enemies() {
    for (Entity i = 0; i < scene->components->entities; i++) {
        EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
        if (!enemy) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Vector3 pos = get_position(i);
        draw_line(pos, add3(pos, enemy->desired_direction), 0.1f, COLOR_GREEN);

        draw_line(pos, add3(pos, rb->angular_velocity), 0.1f, COLOR_RED);
    }
}


void spawn_enemy(Entity trigger, Entity player) {
    UNUSED(player);

    if (scene->enemy != NULL_ENTITY) {
        return;
    }

    float distance = randf(10.0f, 20.0f);
    Vector3 forward = get_axes(trigger).back;
    Vector3 spawn_pos = add3(get_position(trigger), mul3(distance, forward));
    scene->enemy = create_enemy(spawn_pos, get_yaw(trigger) + 90.0f);

    if (in_player_view(player, scene->enemy, INFINITY, 1.0f)) {
        destroy_entity_recursive(scene->enemy);
        scene->enemy = NULL_ENTITY;
    } else {
        destroy_entity_recursive(trigger);
    }
}


void create_window_scare(Vector3 position, float yaw) {
    Entity entity = create_entity();
    TransformComponent_add(entity, (TransformParameters) {
        .position = position,
        .yaw = yaw,
    });
    TriggerComponent_add(entity, (TriggerParameters) {
        .type = TRIGGER_COLLISION,
        .trigger_group = GROUP_PLAYERS,
        .on_enter = spawn_enemy,
        .level = 1
    });
    ColliderComponent* collider = ColliderComponent_add(entity, (ColliderParameters) {
        .type = COLLIDER_CUBOID
    });
    collider->group = GROUP_NONE;
}


void create_corner_scare(Vector3 position, float yaw) {
    Entity entity = create_entity();
    TransformComponent_add(entity, (TransformParameters) {
        .position = position,
        .yaw = yaw,
    });
    TriggerComponent_add(entity, (TriggerParameters) {
        .type = TRIGGER_LOOK,
        .trigger_group = GROUP_PLAYERS,
        .distance = 10.0f,
        .roi = 1.0f,
        .on_enter = spawn_enemy,
        .level = 2
    });
}
