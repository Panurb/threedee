#pragma once

#include "util.h"


#define MAX_PARTICLES 10000


typedef struct ParticleData {
    Vector3 position[MAX_PARTICLES];
    Vector3 velocity[MAX_PARTICLES];
    float time[MAX_PARTICLES];
    int particle_type[MAX_PARTICLES];
    float scale[MAX_PARTICLES];
    int size;
} ParticleData;


ParticleData* ParticleData_create();

void add_particles(int count, Vector3 position, Vector3 velocity, float size, int particle_type);

void update_particles(float time_step);

void update_emitters(float time_step);

void draw_particles();
