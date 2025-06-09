#include <stdlib.h>

#include "components/rigidbody.h"
#include "scene.h"


RigidBodyComponent* RigidBodyComponent_add(Entity entity, float mass) {
    RigidBodyComponent* rigid_body = malloc(sizeof(RigidBodyComponent));

    rigid_body->velocity = zeros3();
    rigid_body->acceleration = zeros3();
    rigid_body->angular_velocity = zeros3();
    rigid_body->angular_acceleration = zeros3();
    rigid_body->mass = mass;
    rigid_body->friction = 1.0f;
    rigid_body->bounce = 0.5f;
    rigid_body->asleep = false;

    scene->components->rigid_body[entity] = rigid_body;

    return rigid_body;
}


void RigidBodyComponent_remove(Entity entity) {
    RigidBodyComponent* ridig_body = scene->components->rigid_body[entity];
    if (ridig_body) {
        free(ridig_body);
        scene->components->rigid_body[entity] = NULL;
    }
}
