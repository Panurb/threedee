#pragma once

#include "util.h"


typedef struct PlayerComponent {
    float yaw;
    float pitch;
    Entity grabbed_entity;
} PlayerComponent;


PlayerComponent* PlayerComponent_add(Entity entity);

void PlayerComponent_remove(Entity entity);
