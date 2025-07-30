#pragma once

#include "util.h"
#include "list.h"


typedef enum EnemyState {
    ENEMY_IDLE,
    ENEMY_WANDER,
    ENEMY_INVESTIGATE,
    ENEMY_CHASE,
    ENEMY_ATTACK,
    ENEMY_DEAD
} EnemyState;


typedef struct EnemyComponent {
    EnemyState state;
    float acceleration;
    Entity target;
    List* path;
    float fov;
    float vision_range;
    float idle_speed;
    float walk_speed;
    float run_speed;
    int weapon;
    Vector3 desired_direction;
    float attack_delay;
    float attack_timer;
    float turn_speed;
    bool boss;
} EnemyComponent;


EnemyComponent* EnemyComponent_add(Entity entity);


void EnemyComponent_remove(Entity entity);
