#include <stdio.h>

#include "scene.h"
#include "systems/collision.h"
#include "systems/physics.h"

#include <math.h>
#include <render.h>

#include "components/rigidbody.h"


static int ITERATIONS = 10;
static Vector3 gravity = { 0.0f, -9.81f, 0.0f };


Matrix3 inertia_tensor(Entity entity) {
    RigidBodyComponent* rigid_body = get_component(entity, COMPONENT_RIGIDBODY);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);

    Matrix3 tensor = (Matrix3) { 0 };

    switch (collider->type) {
        case COLLIDER_SPHERE: {
            float radius = get_radius(entity);
            tensor._11 = (2.0f / 5.0f) * radius * radius / rigid_body->inv_mass;
            tensor._22 = tensor._11;
            tensor._33 = tensor._11;
            break;
        }
        case COLLIDER_PLANE:
            // Plane has no inertia
            break;
        case COLLIDER_CUBE:
            float m = 1.0f / rigid_body->inv_mass;
            float w = collider->radius * get_scale(entity).x;
            float h = collider->radius * get_scale(entity).y;
            float d = collider->radius * get_scale(entity).z;
            tensor._11 = (m / 12.0f) * (h * h + d * d);
            tensor._22 = (m / 12.0f) * (w * w + d * d);
            tensor._33 = (m / 12.0f) * (w * w + h * h);
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
    rb->velocity = sum3(rb->velocity, mult3(rb->inv_mass, impulse));

    // Angular velocity update
    Vector3 torque = cross(r, impulse);
    rb->angular_velocity = sum3(rb->angular_velocity, matrix3_map(rb->inv_inertia, torque));

    rb->asleep = false;
}


void resolve_collisions(Entity entity, float bias) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);

    if (collider) {
        for (int i = 0; i < collider->collisions->size; i++) {
            Collision collision = *(Collision*)ArrayList_get(collider->collisions, i);

            // Only resolve each collision once
            if (collision.entity > entity) continue;

            RigidBodyComponent* rb_other = get_component(collision.entity, COMPONENT_RIGIDBODY);

            Vector3 delta_position = mult3(bias, collision.overlap);

            Vector3 n = normalized3(collision.overlap);
            Vector3 r = collision.offset;
            Vector3 v_rel = sum3(rb->velocity, cross(rb->angular_velocity, r));

            Vector3 r_other = zeros3();
            if (rb_other) {
                r_other = collision.offset_other;
                Vector3 v_other = sum3(rb_other->velocity, cross(rb_other->angular_velocity, r_other));
                v_rel = diff3(v_rel, v_other);
                delta_position = mult3(0.5f, delta_position);
            }

            Vector3 v_n = proj3(v_rel, n);
            Vector3 v_t = diff3(v_rel, v_n);
            Vector3 t = normalized3(v_t);

            if (dot3(v_rel, n) >= 0.0f) {
                // Rigid bodies are separating
                continue;
            }

            // Normal impulse
            float j_n = -(1.0f + rb->bounce) * dot3(v_rel, n);
            float denom_n = rb->inv_mass + dot3(n, cross(matrix3_map(rb->inv_inertia, cross(r, n)), r));

            if (rb_other) {
                denom_n += rb_other->inv_mass + dot3(n, cross(matrix3_map(rb_other->inv_inertia, cross(r_other, n)), r_other));
            }
            j_n /= denom_n;

            // Tangential impulse
            float j_t = -dot3(v_t, t);
            float denom_t = (rb->inv_mass + dot3(t, cross(matrix3_map(rb->inv_inertia, cross(r, t)), r)));

            if (rb_other) {
                denom_t += (rb_other->inv_mass + dot3(t, cross(matrix3_map(rb_other->inv_inertia, cross(r_other, t)), r_other)));
            }
            j_t /= denom_t;

            // Clamp according to Coulomb's law of friction
            float j_t_max = rb->friction * fabsf(j_n);
            if (fabsf(j_t) > j_t_max) {
                j_t = j_t_max * sign(j_t);
            }

            if (fabsf(j_n) < 0.01f && fabsf(j_t) < 0.01f) {
                // If impulse is negligible, skip the collision resolution
                continue;
            }

            // Total impulse
            Vector3 j_total = sum3(mult3(j_n, n), mult3(j_t, t));

            // TODO: What if entity has parent?
            trans->position = sum3(trans->position, delta_position);

            apply_impulse(entity, sum3(trans->position, r), j_total);

            if (rb_other) {
                TransformComponent* trans_other = get_component(collision.entity, COMPONENT_TRANSFORM);
                trans_other->position = sum3(trans_other->position, mult3(-1.0f, delta_position));

                apply_impulse(collision.entity, sum3(trans_other->position, r_other), mult3(-1.0f, j_total));
            }
        }
    }
}


void init_physics(void) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        RigidBodyComponent* rigid_body = get_component(i, COMPONENT_RIGIDBODY);
        if (rigid_body) {
            rigid_body->inv_inertia = matrix3_inverse(inertia_tensor(i));
        }
    }
}


void update_physics(float time_step) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        RigidBodyComponent* rigid_body = scene->components->rigid_body[i];
        if (!rigid_body) continue;

        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);

        for (int j = 0; j < ITERATIONS; j++) {
            resolve_collisions(i, 1.0f / (float)ITERATIONS);
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

        // Clamp velocities
        rigid_body->velocity = clamp_magnitude3(rigid_body->velocity, 0.0f, rigid_body->max_speed);
        rigid_body->angular_velocity = clamp_magnitude3(rigid_body->angular_velocity, 0.0f, rigid_body->max_angular_speed);

        // Apply damping
        rigid_body->velocity = mult3(rigid_body->linear_damping, rigid_body->velocity);
        rigid_body->angular_velocity = mult3(rigid_body->angular_damping, rigid_body->angular_velocity);

        if (norm3(rigid_body->velocity) < 0.001f && norm3(rigid_body->angular_velocity) < 0.01f) {
            rigid_body->velocity = zeros3();
            rigid_body->angular_velocity = zeros3();
            rigid_body->asleep = true;
            LOG_INFO("Rigid body %d is asleep", i);
        }

        rigid_body->acceleration = zeros3();
        rigid_body->angular_acceleration = zeros3();
    }
}
