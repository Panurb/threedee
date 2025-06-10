#include <stdlib.h>

#include "components/rigidbody.h"
#include "scene.h"


RigidBodyComponent* RigidBodyComponent_add(Entity entity, float mass) {
    RigidBodyComponent* rigid_body = malloc(sizeof(RigidBodyComponent));

    rigid_body->velocity = zeros3();
    rigid_body->acceleration = zeros3();
    rigid_body->angular_velocity = zeros3();
    rigid_body->angular_acceleration = zeros3();
    rigid_body->inv_mass = 1.0f / mass;
    rigid_body->friction = 0.5f;
    rigid_body->bounce = 0.5f;
    rigid_body->asleep = false;
    rigid_body->angular_damping = 0.95f;
    rigid_body->linear_damping = 0.999f;
    rigid_body->inv_inertia = matrix3_id();
    rigid_body->max_speed = 10.0f;
    rigid_body->max_angular_speed = 2.0f;

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
