#include "systems/enemy.h"
#include "components/enemy.h"
#include "scene.h"
#include "util.h"
#include "raycast.h"
#include "systems/navigation.h"


Entity create_enemy(Vector3 pos, float yaw) {
    Entity i = create_entity();

    TransformComponent_add(i, (TransformParameters) {
        .position = pos,
        .yaw = yaw
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_CAPSULE,
        .radius = 0.5f,
        .height = 1.0f,
        .group = GROUP_ENEMIES
    });

    MeshComponent_add(i, (MeshParameters) {
       .mesh_filename = "teapot"
    });
    RigidBodyComponent* rb = RigidBodyComponent_add(i, 1.0f);
    rb->axis_lock.rotation = true;
    EnemyComponent_add(i);
    WaypointComponent_add(i);
    SoundComponent_add(i, (SoundParameters) {});

    return i;
}


void update_enemies(float time_step) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        EnemyComponent* enemy = get_component(i, COMPONENT_ENEMY);
        if (!enemy) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);
        Vector3 pos = get_position(i);

        if (enemy->state != ENEMY_ATTACK && enemy->state != ENEMY_DEAD) {
            // update_vision(i);
            turn_to(i, enemy->desired_direction, enemy->turn_speed * time_step);
        }

        switch (enemy->state) {
            case ENEMY_IDLE: {
                // enemy->desired_direction = zeros3();
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
