#pragma once

#include <arraylist.h>

#include "util.h"


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
    float head_height;
    float view_bobbing;
} PlayerComponent;


PlayerComponent* PlayerComponent_add(Entity entity);

void PlayerComponent_remove(Entity entity);
