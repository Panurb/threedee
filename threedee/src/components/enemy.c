#define _USE_MATH_DEFINES

#include "components/enemy.h"
#include "scene.h"


EnemyComponent* EnemyComponent_add(Entity entity) {
    EnemyComponent* enemy = malloc(sizeof(EnemyComponent));
    enemy->state = ENEMY_IDLE;
    enemy->acceleration = 15.0f;
    enemy->target = -1;
    enemy->path = List_create();
    enemy->fov = M_PI_2;
    enemy->vision_range = 15.0f;
    enemy->idle_speed = 1.0f;
    enemy->walk_speed = 0.5f;
    enemy->run_speed = 2.0f;
    enemy->weapon = -1;
    enemy->desired_direction = quaternion_forward(get_rotation(entity));
    enemy->attack_delay = 0.1f;
    enemy->attack_timer = enemy->attack_delay;
    enemy->turn_speed = 5.0f;
    enemy->boss = false;

    scene->components->enemy[entity] = enemy;

    return enemy;
}


void EnemyComponent_remove(Entity entity) {
    EnemyComponent* enemy = get_component(entity, COMPONENT_ENEMY);
    if (enemy) {
        List_delete(enemy->path);
        free(enemy);
        scene->components->enemy[entity] = NULL;
    }
}
