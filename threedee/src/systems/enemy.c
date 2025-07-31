#include "systems/enemy.h"
#include "components/enemy.h"

#include <stdio.h>

#include "render.h"
#include "scene.h"
#include "util.h"
#include "raycast.h"
#include "systems/navigation.h"


Entity create_enemy(Vector3 pos, float yaw) {
    Entity i = create_entity();

    TransformComponent_add(i, (TransformParameters) {
        .position = add3(pos, vec3(0.0f, 1.0f, 0.0f)),
        .yaw = yaw
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_SPHERE,
        .radius = 1.0f,
        .group = GROUP_ENEMIES
    });

    MeshComponent_add(i, (MeshParameters) {
       .mesh_filename = "cube"
    });
    RigidBodyComponent* rb = RigidBodyComponent_add(i, 1.0f);
    rb->axis_lock.rotation = true;
    rb->friction = 1.0f;
    EnemyComponent_add(i);
    WaypointComponent_add(i);
    SoundComponent_add(i, (SoundParameters) {});

    return i;
}


void update_vision(int entity) {
    EnemyComponent* enemy = get_component(entity, COMPONENT_ENEMY);

    Vector3 pos = get_position(entity);

    Vector3 r = sub3(get_position(scene->player), pos);
    float angle = dot3(normalized3(r), quaternion_forward(get_rotation(entity)));

    float d = norm3(r);
    if (d < enemy->vision_range && angle < 0.5f * cosf(enemy->fov)) {
        Hit info = raycast((Ray) { pos, r, enemy->vision_range }, GROUP_PLAYERS | GROUP_WALLS);
        if (info.entity == scene->player) {
            enemy->target = scene->player;
            enemy->state = ENEMY_CHASE;
        }
    }
}


void update_enemies(float time_step) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
        if (!enemy) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);
        Vector3 pos = get_position(i);

        if (enemy->state != ENEMY_ATTACK && enemy->state != ENEMY_DEAD) {
            update_vision(i);
            Vector3 dir = vec3(enemy->desired_direction.x, pos.y, enemy->desired_direction.z);
            turn_to(i, dir, enemy->turn_speed * time_step);
        }

        switch (enemy->state) {
            case ENEMY_IDLE: {
                enemy->desired_direction = quaternion_forward(get_rotation(i));
                break;
            } case ENEMY_WANDER: {
                break;
            } case ENEMY_INVESTIGATE: {
                a_star(i, enemy->target, enemy->path);

                if (enemy->path->size > 1) {
                    enemy->desired_direction = get_position(enemy->path->head->next->value);
                } else {
                    enemy->desired_direction = get_position(enemy->target);
                }

                Vector3 r = sub3(enemy->desired_direction, pos);

                float speed = norm3(rb->velocity);
                if (speed < enemy->walk_speed) {
                    rb->acceleration = add3(rb->acceleration, mul3(enemy->acceleration, normalized3(r)));
                }

                break;
            } case ENEMY_CHASE: {
                a_star(i, enemy->target, enemy->path);

                if (enemy->path->size > 1) {
                    enemy->desired_direction = get_position(enemy->path->head->next->value);
                } else {
                    enemy->desired_direction = get_position(enemy->target);
                }

                Vector3 r = sub3(enemy->desired_direction, pos);

                float speed = norm3(rb->velocity);
                if (speed < enemy->run_speed) {
                    rb->acceleration = add3(rb->acceleration, mul3(enemy->acceleration, normalized3(r)));
                }

                break;
            } case ENEMY_ATTACK: {
                if (enemy->attack_timer <= 0.0f) {
                    // attack(enemy->weapon);
                    enemy->state = ENEMY_CHASE;
                } else {
                    enemy->attack_timer -= time_step;
                }

                break;
            } case ENEMY_DEAD: {
                break;
            }
        }
    }
}


void debug_draw_enemies() {
    for (Entity i = 0; i < scene->components->entities; i++) {
        EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
        if (!enemy) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Vector3 pos = get_position(i);
        render_line(pos, add3(pos, enemy->desired_direction), 0.1f, COLOR_GREEN);

        render_line(pos, add3(pos, rb->angular_velocity), 0.1f, COLOR_RED);
    }
}
