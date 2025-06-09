#include <stdio.h>

#include "scene.h"
#include "systems/collision.h"
#include "systems/physics.h"

#include <math.h>

#include "components/rigidbody.h"


static Vector3 gravity = { 0.0f, -9.81f, 0.0f };


float moment_of_inertia(Entity entity) {
    RigidBodyComponent* rigid_body = get_component(entity, COMPONENT_RIGIDBODY);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);

    switch (collider->type) {
        case COLLIDER_SPHERE: {
            float radius = get_radius(entity);
            return (2.0f / 5.0f) * rigid_body->mass * radius * radius;
        }
        case COLLIDER_PLANE:
            return 0.0f;
        default:
            LOG_ERROR("Unknown collider type for moment of inertia calculation");
            return 0.0f;
    }
}


void apply_impulse(Entity entity, Vector3 point, Vector3 impulse) {
    RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
    if (!rb) return;

    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);

    // Offset from center of mass to contact point
    Vector3 r = diff3(point, trans->position);

    // Linear velocity update
    rb->velocity = sum3(rb->velocity, mult3(1.0f / rb->mass, impulse));

    // Angular velocity update
    float inertia = moment_of_inertia(entity);
    Vector3 torque = cross(r, impulse);
    rb->angular_velocity = sum3(rb->angular_velocity, mult3(1.0f / inertia, torque));
}


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
                Vector3 normal = normalized3(collision.overlap);

                Vector3 r = mult3(-collider->radius, normal);
                Vector3 v_rel = sum3(rigid_body->velocity, cross(rigid_body->angular_velocity, r));
                Vector3 v_n = proj3(v_rel, normal);
                Vector3 v_t = diff3(v_rel, v_n);
                Vector3 tangent = normalized3(v_t);

                // TODO: Use inertia tensor
                float inertia = moment_of_inertia(i);

                // Normal impulse
                float j_n = 0.0f;
                if (dot3(v_n, normal) < 0.0f) {
                    j_n = -(1.0f + rigid_body->bounce) * dot3(v_rel, normal);
                    j_n /= (1.0f / rigid_body->mass + normsqr3(cross(r, normal)) / inertia);
                }

                // Tangential impulse
                float j_t = -dot3(v_t, tangent);
                j_t /= (1.0f / rigid_body->mass + normsqr3(cross(r, tangent)) / inertia);

                // Clamp according to Coulomb's law of friction
                float j_t_max = rigid_body->friction * fabsf(j_n);
                if (fabsf(j_t) > j_t_max) {
                    j_t = -j_t_max * sign(j_t);
                }

                // Total impulse
                Vector3 j_total = sum3(mult3(j_n, normal), mult3(j_t, tangent));

                rigid_body->velocity = sum3(rigid_body->velocity, mult3(1.0f / rigid_body->mass, j_total));

                Vector3 torque = cross(r, j_total);
                rigid_body->angular_velocity = sum3(rigid_body->angular_velocity, mult3(1.0f / inertia, torque));

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

        if (norm3(rigid_body->velocity) < 1e-6f && norm3(rigid_body->angular_velocity) < 1e-6f) {
            rigid_body->velocity = zeros3();
            rigid_body->angular_velocity = zeros3();
            rigid_body->asleep = true;
        }

        rigid_body->acceleration = zeros3();
        rigid_body->angular_acceleration = zeros3();
    }
}
