#pragma once

#include "util.h"
#include "linalg.h"


#define MAX_PARTICLES 1000
#define MAX_PHASES 10


typedef struct ParticlePhase {
    Color color;
    float size;
    float normalized_time;
} ParticlePhase;


typedef struct ParticleParameters {
    float lifetime;
    ParticlePhase phases[MAX_PHASES];
    int num_phases;
    float gravity_scale;
} ParticleParameters;


typedef struct ParticleComponent {
    float lifetime;
    Vector3 position[MAX_PARTICLES];
    Vector3 velocity[MAX_PARTICLES];
    float time[MAX_PARTICLES];
    ParticlePhase phases[MAX_PHASES];
    int num_phases;
    int num_particles;
    float gravity_scale;
} ParticleComponent;


ParticleComponent* ParticleComponent_add(Entity entity, ParticleParameters params);

void ParticleComponent_add_phase(Color color, float size, float time);

void ParticleComponent_remove(Entity entity);
