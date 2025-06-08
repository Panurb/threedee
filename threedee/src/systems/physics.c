#include "scene.h"
#include "systems/physics.h"
#include "components/rigidbody.h"


static Vector3 gravity = { 0.0f, -9.81f, 0.0f };


void update_physics(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        RigidBodyComponent* rigid_body = scene->components->rigid_body[i];
        if (!rigid_body) continue;

        TransformComponent* trans = TransformComponent_get(i);
        ColliderComponent* collider = scene->components->collider[i];

        if (collider) {
            for (int j = 0; j < collider->collisions->size; j++) {
                Collision collision = *(Collision*)ArrayList_get(collider->collisions, j);
                trans->position = sum3(trans->position, collision.overlap);
                rigid_body->velocity = mult3(-rigid_body->bounce, rigid_body->velocity);
                rigid_body->asleep = false;
            }
        }

        if (rigid_body->asleep) {
            continue;
        }

        rigid_body->acceleration = sum3(rigid_body->acceleration, gravity);
        rigid_body->velocity = sum3(rigid_body->velocity, mult3(time_step, rigid_body->acceleration));
        trans->position = sum3(trans->position, mult3(time_step, rigid_body->velocity));

        rigid_body->angular_velocity = sum3(rigid_body->angular_velocity, mult3(time_step, rigid_body->angular_acceleration));
        float angle = norm3(rigid_body->angular_velocity) * time_step;
        Vector3 axis = normalized3(rigid_body->angular_velocity);
        Quaternion delta_rotation = axis_angle_to_quaternion(axis, angle);
        trans->rotation = quaternion_mult(delta_rotation, trans->rotation);

        if (norm3(rigid_body->velocity) < 1e-6f) {
            rigid_body->velocity = zeros3();
            rigid_body->asleep = true;
        }

        rigid_body->acceleration = zeros3();
        rigid_body->angular_acceleration = zeros3();
    }
}
