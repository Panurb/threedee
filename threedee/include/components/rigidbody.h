#pragma once

#include "util.h"


typedef struct {
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 angular_velocity;
    Vector3 angular_acceleration;
    float friction;
    float bounce;
    float inv_mass;
    Matrix3 inv_inertia;
    bool asleep;
} RigidBodyComponent;


RigidBodyComponent* RigidBodyComponent_add(Entity entity, float mass);

void RigidBodyComponent_remove(Entity entity);
