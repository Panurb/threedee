#pragma once

#include "util.h"
#include "linalg.h"


typedef struct EmitterParameters {
    float spawn_rate;
    Vector3 position_variance;
    Vector3 velocity;
    Vector3 velocity_variance;
    float speed;
    String particle_type_name;
    float scale;
} EmitterParameters;


typedef struct EmitterComponent {
    float spawn_rate;  // particles per second
    Vector3 spawn_velocity;
    Vector3 velocity_variance;
    float speed;
    float spawn_accumulator;
    Vector3 position_variance;
    float scale;
    int particle_type;
} EmitterComponent;


EmitterComponent* EmitterComponent_add(Entity entity, EmitterParameters params);

void EmitterComponent_remove(Entity entity);
