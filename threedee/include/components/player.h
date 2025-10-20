#pragma once

#include <arraylist.h>

#include "util.h"


typedef enum Foot {
    FOOT_LEFT = -1,
    FOOT_BOTH = 0,
    FOOT_RIGHT = 1
} Foot;


typedef struct PlayerComponent {
    float yaw;
    float pitch;
    Entity grabbed_entity;
    Vector3 grabbed_position;
    Quaternion grabbed_rotation;
    bool examining;
    float examine_yaw;
    ArrayList* inventory;
    int selected_item;
    float footstep_timer;
    float footstep_interval;
    Foot foot;
    float head_height;
    float view_bobbing;
    bool sprinting;
    float walk_speed;
    float sprint_speed;
} PlayerComponent;


PlayerComponent* PlayerComponent_add(Entity entity);

void PlayerComponent_remove(Entity entity);
