#include <stdlib.h>

#include "components/particle.h"
#include "scene.h"
#include "resources.h"


ParticleComponent* ParticleComponent_add(Entity entity, ParticleParameters params) {
    ParticleComponent* particle = malloc(sizeof(ParticleComponent));

    particle->spawn_velocity = params.velocity;
    particle->velocity_variance = params.velocity_variance;
    particle->position_variance = params.position_variance;
    particle->scale = params.scale ? params.scale : 1.0f;

    particle->spawn_rate = params.spawn_rate;
    particle->spawn_accumulator = 0.0f;

    if (params.particle_type_name[0] != '\0') {
        particle->particle_type = binary_search_filename(
            params.particle_type_name,
            resources.particle_type_names,
            resources.particle_types_size
        );
    }

    scene->components->particle[entity] = particle;
    return particle;
}


void ParticleComponent_remove(Entity entity) {
    ParticleComponent* particle = get_component(entity, COMPONENT_PARTICLE);
    if (particle) {
        free(particle);
        scene->components->particle[entity] = NULL;
    }
}
