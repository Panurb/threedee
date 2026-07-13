#include <stdlib.h>
#include <stdio.h>

#include "scene.h"
#include "components/force.h"


ForceComponent* ForceComponent_add(Entity entity, ForceParameters params) {
    ForceComponent* force = malloc(sizeof(ForceComponent));
    force->enabled = !params.disabled;
    force->direction = normalized3(params.direction);
    force->magnitude = params.magnitude;
    force->target_group = params.target_group ? params.target_group : GROUP_ALL;
    scene->components->force[entity] = force;
    return force;
}


void ForceComponent_remove(Entity entity) {
    ForceComponent* force = get_component(entity, COMPONENT_FORCE);
    if (force) {
        free(force);
        scene->components->force[entity] = NULL;
    }
}


void enable_force(Entity trigger, Entity entity) {
    UNUSED(entity);
    ForceComponent* force = get_component(trigger, COMPONENT_FORCE);
    if (force) {
        force->enabled = true;
    }
}


void disable_force(Entity trigger, Entity entity) {
    UNUSED(entity);
    ForceComponent* force = get_component(trigger, COMPONENT_FORCE);
    if (force) {
        force->enabled = false;
    }
}
