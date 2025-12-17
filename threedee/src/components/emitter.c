#include <stdlib.h>
#include <stdio.h>

#include "components/emitter.h"
#include "scene.h"
#include "resources.h"


EmitterComponent* EmitterComponent_add(Entity entity, EmitterParameters params) {
    EmitterComponent* emitter = malloc(sizeof(EmitterComponent));

    emitter->spawn_velocity = params.velocity;
    emitter->velocity_variance = params.velocity_variance;
    emitter->position_variance = params.position_variance;
    emitter->scale = params.scale ? params.scale : 1.0f;

    emitter->spawn_rate = params.spawn_rate;
    emitter->spawn_accumulator = 0.0f;

    if (params.particle_type_name[0] != '\0') {
        emitter->particle_type = binary_search_filename(
            params.particle_type_name,
            resources.particle_type_names,
            resources.particle_types_size
        );
    }
    if (emitter->particle_type == -1) {
        LOG_WARNING("Particle type '%s' not found", params.particle_type_name);
    }

    scene->components->emitter[entity] = emitter;
    return emitter;
}


void EmitterComponent_remove(Entity entity) {
    EmitterComponent* emitter = get_component(entity, COMPONENT_EMITTER);
    if (emitter) {
        free(emitter);
        scene->components->emitter[entity] = NULL;
    }
}
