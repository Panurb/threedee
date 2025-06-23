#pragma once

#include "util.h"


typedef struct {
    bool x;
    bool y;
    bool z;
    bool rotation;
    Vector3 rotation_axis;
} AxisLock;


typedef struct {
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 angular_velocity;
    Vector3 angular_acceleration;
    float friction;
    float bounce;
    float inv_mass;
    float gravity_scale;
    Matrix3 inv_inertia;
    bool asleep;
    float linear_damping;
    float angular_damping;
    float max_speed;
    float max_angular_speed;
    AxisLock axis_lock;
} RigidBodyComponent;


RigidBodyComponent* RigidBodyComponent_add(Entity entity, float mass);

void RigidBodyComponent_remove(Entity entity);
