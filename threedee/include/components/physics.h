#pragma once

#include "util.h"


typedef struct {
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 angular_velocity;
    float mass;
    float friction;
    float bounce;
    bool asleep;
} PhysicsComponent;


PhysicsComponent* PhysicsComponent_add(Entity entity, float mass);

void PhysicsComponent_remove(Entity entity);
