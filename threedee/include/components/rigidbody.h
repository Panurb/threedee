#pragma once

#include "util.h"
#include "arraylist.h"


typedef struct {
    bool x;
    bool y;
    bool z;
    bool rotation;
    Vector3 rotation_axis;
} AxisLock;


typedef struct Spring {
    Entity entity;
    Vector3 local_anchor;
    Vector3 other_local_anchor;
    float rest_length;
    float stiffness;
    float damping;
    Color color;
    float thickness;
} Spring;


typedef struct RigidBodyParameters {
    float mass;
    float friction;
    float bounce;
    float gravity_scale;
    AxisLock axis_lock;
    bool dont_sleep;
    String move_sound;
} RigidBodyParameters;


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
    bool can_sleep;
    bool asleep;
    bool on_ground;
    float linear_damping;
    float angular_damping;
    float max_speed;
    float max_angular_speed;
    AxisLock axis_lock;
    ArrayList* springs;
    String move_sound;
} RigidBodyComponent;


RigidBodyComponent* RigidBodyComponent_add(Entity entity, RigidBodyParameters params);

void RigidBodyComponent_remove(Entity entity);

void add_spring(Entity entity, Spring spring);
