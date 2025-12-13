#pragma once

#include "util.h"
#include "linalg.h"


typedef struct ParticleParameters {
    float spawn_rate;
    Vector3 position_variance;
    Vector3 velocity;
    Vector3 velocity_variance;
    float speed;
    String particle_type_name;
    float scale;
} ParticleParameters;


typedef struct ParticleComponent {
    float spawn_rate;  // particles per second
    Vector3 spawn_velocity;
    Vector3 velocity_variance;
    float speed;
    float spawn_accumulator;
    Vector3 position_variance;
    float scale;
    int particle_type;
} ParticleComponent;


ParticleComponent* ParticleComponent_add(Entity entity, ParticleParameters params);

void ParticleComponent_add_phase(Color color, float size, float time);

void ParticleComponent_remove(Entity entity);
