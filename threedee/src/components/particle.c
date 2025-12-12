#include <stdlib.h>
#include <stdio.h>

#include "components/particle.h"
#include "scene.h"
#include "resources.h"


ParticleComponent* ParticleComponent_add(Entity entity, ParticleParameters params) {
    ParticleComponent* particle = malloc(sizeof(ParticleComponent));
    particle->lifetime = params.lifetime ? params.lifetime : 1.0f;
    particle->num_particles = 0;
    particle->num_phases = 0;
    memset(particle->position, 0, sizeof(particle->position));
    memset(particle->velocity, 0, sizeof(particle->velocity));
    memset(particle->time, 0, sizeof(particle->time));
    particle->gravity_scale = params.gravity_scale;
    particle->texture_index = -1;
    particle->spawn_rate = params.spawn_rate;
    particle->spawn_velocity = params.velocity;
    particle->velocity_variance = params.velocity_variance;
    particle->spawn_accumulator = 0.0f;
    particle->position_variance = params.position_variance;
    particle->emissive = params.emissive;

    if (params.texture_name[0] != '\0') {
        particle->texture_index = binary_search_filename(params.texture_name, resources.particle_names, resources.particles_size);
    }

    for (int i = 0; i < params.num_phases && i < MAX_PHASES; i++) {
        particle->phases[i] = params.phases[i];
        particle->num_phases++;
    }

    scene->components->particle[entity] = particle;
    return particle;
}


void ParticleComponent_add_phase(Color color, float size, float time) {
    ParticleComponent* particle = scene->components->particle[0]; // Assuming a single global particle component for phases
    if (particle->num_phases >= MAX_PHASES) {
        LOG_WARNING("Maximum number of particle phases reached.");
        return;
    }

    ParticlePhase* phase = &particle->phases[particle->num_phases++];
    phase->color = color;
    phase->size = size;
    phase->normalized_time = time;
}


void ParticleComponent_remove(Entity entity) {
    ParticleComponent* particle = get_component(entity, COMPONENT_PARTICLE);
    if (particle) {
        free(particle);
        scene->components->particle[entity] = NULL;
    }
}
