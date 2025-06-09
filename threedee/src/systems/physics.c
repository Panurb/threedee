#include <stdio.h>

#include "scene.h"
#include "systems/collision.h"
#include "systems/physics.h"

#include <math.h>

#include "components/rigidbody.h"


static Vector3 gravity = { 0.0f, -9.81f, 0.0f };


Matrix3 inertia_tensor(Entity entity) {
    RigidBodyComponent* rigid_body = get_component(entity, COMPONENT_RIGIDBODY);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);

    Matrix3 tensor = (Matrix3) { 0 };

    switch (collider->type) {
        case COLLIDER_SPHERE: {
            float radius = get_radius(entity);
            tensor.a = (2.0f / 5.0f) * rigid_body->mass * radius * radius;
            tensor.e = tensor.a;
            tensor.i = tensor.a;
            break;
        }
        case COLLIDER_PLANE:
            // Plane has no inertia
            break;
        default:
            LOG_ERROR("Unknown collider type for inertia tensor calculation");
            break;
    }

    return tensor;
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
    Matrix3 inertia = inertia_tensor(entity);
    Vector3 torque = cross(r, impulse);
    rb->angular_velocity = sum3(rb->angular_velocity, matrix3_map(matrix3_inverse(inertia), torque));
}


void resolve_collisions(Entity entity) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    RigidBodyComponent* rigid_body = get_component(entity, COMPONENT_RIGIDBODY);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);

    if (collider) {
        for (int j = 0; j < collider->collisions->size; j++) {
            if (j == entity) break;

            Collision collision = *(Collision*)ArrayList_get(collider->collisions, j);
            RigidBodyComponent* rb_other = get_component(collision.entity, COMPONENT_RIGIDBODY);

            trans->position = sum3(trans->position, mult3(0.5f, collision.overlap));

            Vector3 n = normalized3(collision.overlap);
            Vector3 r = mult3(-get_radius(entity), n);

            Vector3 v_rel = sum3(rigid_body->velocity, cross(rigid_body->angular_velocity, r));
            if (rb_other) {
                Vector3 r_other = mult3(get_radius(collision.entity), n);
                Vector3 v_rel_other = sum3(rb_other->velocity, cross(rb_other->angular_velocity, r_other));
                v_rel = diff3(v_rel, v_rel_other);
            }

            Vector3 v_n = proj3(v_rel, n);
            Vector3 v_t = diff3(v_rel, v_n);
            Vector3 t = normalized3(v_t);

            if (dot3(v_rel, n) >= 0.0f) {
                // Rigid bodies are separating
                continue;
            }

            Matrix3 inertia = inertia_tensor(entity);
            Matrix3 i_inv = matrix3_inverse(inertia);

            // Normal impulse
            float j_n = -(1.0f + rigid_body->bounce) * dot3(v_rel, n);
            float denom_n = 1.0f / rigid_body->mass + dot3(n, cross(matrix3_map(i_inv, cross(r, n)), r));

            if (rb_other) {
                Vector3 r_other = mult3(get_radius(collision.entity), n);
                Matrix3 inertia_other = inertia_tensor(collision.entity);
                Matrix3 i_inv_other = matrix3_inverse(inertia_other);
                denom_n += 1.0f / rb_other->mass + dot3(n, cross(matrix3_map(i_inv_other, cross(r_other, n)), r_other));
            }
            j_n /= denom_n;

            // Tangential impulse
            float j_t = -dot3(v_t, t);
            float denom_t = (1.0f / rigid_body->mass + dot3(t, cross(matrix3_map(i_inv, cross(r, t)), r)));

            if (rb_other) {
                Vector3 r_other = mult3(get_radius(collision.entity), n);
                Matrix3 inertia_other = inertia_tensor(collision.entity);
                Matrix3 i_inv_other = matrix3_inverse(inertia_other);
                denom_t += (1.0f / rb_other->mass + dot3(t, cross(matrix3_map(i_inv_other, cross(r_other, t)), r_other)));
            }
            j_t /= denom_t;

            // Clamp according to Coulomb's law of friction
            float j_t_max = rigid_body->friction * fabsf(j_n);
            if (fabsf(j_t) > j_t_max) {
                j_t = j_t_max * sign(j_t);
            }

            // Total impulse
            Vector3 j_total = sum3(mult3(j_n, n), mult3(j_t, t));

            rigid_body->velocity = sum3(rigid_body->velocity, mult3(1.0f / rigid_body->mass, j_total));

            Vector3 torque = cross(r, j_total);
            rigid_body->angular_velocity = sum3(rigid_body->angular_velocity, matrix3_map(i_inv, torque));

            if (rb_other) {
                rb_other->velocity = sum3(rb_other->velocity, mult3(1.0f / rb_other->mass, mult3(-1.0f, j_total)));

                Vector3 r_other = mult3(get_radius(collision.entity), n);
                Vector3 torque_other = cross(r_other, mult3(-1.0f, j_total));
                rb_other->angular_velocity = sum3(rb_other->angular_velocity, matrix3_map(matrix3_inverse(inertia_tensor(collision.entity)), torque_other));
            }

            rigid_body->asleep = false;
        }
    }
}


void update_physics(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        RigidBodyComponent* rigid_body = scene->components->rigid_body[i];
        if (!rigid_body) continue;

        TransformComponent* trans = TransformComponent_get(i);

        resolve_collisions(i);

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
