#include "systems/physics.h"

#include <scene.h>


static Vector3 gravity = { 0.0f, -9.81f, 0.0f };


void update_physics(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        PhysicsComponent* physics = scene->components->physics[i];
        if (!physics) continue;

        TransformComponent* trans = TransformComponent_get(i);
        ColliderComponent* collider = scene->components->collider[i];

        if (collider) {
            for (int j = 0; j < collider->collisions->size; j++) {
                Collision collision = *(Collision*)ArrayList_get(collider->collisions, j);
                trans->position = sum3(trans->position, collision.overlap);
                physics->velocity = mult3(-physics->bounce, physics->velocity);
                physics->asleep = false;
            }
        }

        if (physics->asleep) {
            continue;
        }

        physics->acceleration = sum3(physics->acceleration, gravity);

        physics->velocity = sum3(physics->velocity, mult3(time_step, physics->acceleration));
        physics->velocity = mult3(1.0f - physics->friction * time_step, physics->velocity);

        trans->position = sum3(trans->position, mult3(time_step, physics->velocity));

        if (norm3(physics->velocity) < 0.0001f) {
            physics->velocity = zeros3();
            physics->asleep = true;
        }

        physics->acceleration = zeros3();
    }
}
