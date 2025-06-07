#include <stdlib.h>

#include "components/physics.h"
#include "scene.h"


PhysicsComponent* PhysicsComponent_add(Entity entity, float mass) {
    PhysicsComponent* physics = malloc(sizeof(PhysicsComponent));

    physics->velocity = zeros3();
    physics->acceleration = zeros3();
    physics->angular_velocity = zeros3();
    physics->mass = mass;
    physics->friction = 0.5f;
    physics->bounce = 0.5f;
    physics->asleep = false;

    scene->components->physics[entity] = physics;

    return physics;
}


void PhysicsComponent_remove(Entity entity) {
    PhysicsComponent* physics = scene->components->physics[entity];
    if (physics) {
        free(physics);
        scene->components->physics[entity] = NULL;
    }
}
