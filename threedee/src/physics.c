#define _USE_MATH_DEFINES

#include <math.h>

#include "component.h"
#include "game.h"
#include "util.h"
#include "grid.h"


void apply_force(int entity, Vector2f force) {
    // TODO: use delta time?
    PhysicsComponent* physics = PhysicsComponent_get(entity);

    Vector2f a = mult(1.0f / physics->mass, force);
    physics->acceleration = sum(physics->acceleration, a);
}


void update_physics(float time_step) {
    for (int i = 0; i < game_data->components->entities; i++) {
        PhysicsComponent* physics = PhysicsComponent_get(i);
        if (!physics) continue;

        ColliderComponent* col = ColliderComponent_get(i);

        if (get_parent(i) != -1) continue;

        if (physics->collision.entities->size > 0) {
            physics->velocity = physics->collision.velocity;

            Vector2f v_n = proj(physics->velocity, physics->collision.overlap);
            Vector2f v_t = diff(physics->velocity, v_n);

            physics->velocity = sum(mult(physics->bounce, v_n), mult(1.0f - physics->friction, v_t));
        }

        Vector2f delta_pos = sum(physics->collision.overlap, mult(time_step, physics->velocity));
        float delta_angle = time_step * physics->angular_velocity;

        CoordinateComponent* coord = CoordinateComponent_get(i);
        JointComponent* joint = JointComponent_get(i);
        if (joint && joint->parent != -1) {
            Vector2f parent_position = CoordinateComponent_get(joint->parent)->position;
            Vector2f r = diff(parent_position, coord->position);
            float d = norm(r);

            Vector2f f = zeros();
            if (d > joint->max_length) {
                f = mult(d - joint->max_length, normalized(r));
            } else if (d < joint->min_length) {
                f = mult(d - joint->min_length, normalized(r));
            }
            delta_pos = sum(delta_pos, mult(joint->strength, f));
            
            delta_angle = angle_diff(polar_angle(r), coord->angle);

            float parent_angle = CoordinateComponent_get(joint->parent)->angle;
            float angle = signed_angle(r, polar_to_cartesian(1.0f, parent_angle));
            if (fabsf(angle) > joint->max_angle) {
                r = polar_to_cartesian(d, parent_angle - sign(angle) * joint->max_angle);
                delta_pos = sum(delta_pos, diff(diff(parent_position, r), coord->position));
            }
        }

        Vector2f v_hat = normalized(physics->velocity);
        Vector2f v_forward = proj(v_hat, polar_to_cartesian(1.0, coord->angle));
        Vector2f v_sideways = diff(v_hat, v_forward);
        Vector2f a = lin_comb(-physics->drag, v_forward, -physics->drag_sideways, v_sideways);
        physics->acceleration = sum(physics->acceleration, a);
        physics->velocity = sum(physics->velocity, mult(time_step, physics->acceleration));
        physics->acceleration = zeros();

        switch (physics->lock) {
            case AXIS_NONE:
                break;
            case AXIS_POSITION:
                delta_angle = 0.0f;
                physics->angular_velocity = 0.0f;
                break;
            case AXIS_ANGLE:
                delta_pos = zeros();
                physics->velocity = zeros();
                break;
            case AXIS_ALL:
                delta_pos = zeros();
                delta_angle = 0.0f;
                physics->velocity = zeros();
                physics->angular_velocity = 0.0f;
                break;
        }

        bool moved = col && col->enabled && (non_zero(delta_pos) || delta_angle != 0.0f);
        if (moved) {
            clear_grid(i);
        }
        coord->position = sum(coord->position, delta_pos);
        coord->angle = mod(coord->angle + delta_angle, 2.0f * M_PI);
        if (moved) {
            update_grid(i);
        }
        
        physics->speed = norm(physics->velocity);
        float max_speed = physics->max_speed;
        if (physics->slowed && !physics->on_ground) {
            max_speed *= 0.6f;
        }
        if (physics->speed < 0.1f) {
            physics->velocity = zeros();
            physics->speed = 0.0f;
        } else if (physics->speed > max_speed) {
            physics->velocity = mult(max_speed / physics->speed, physics->velocity);
            physics->speed = max_speed;
        }

        physics->angular_acceleration -= sign(physics->angular_velocity) * physics->angular_drag;
        physics->angular_velocity += time_step * physics->angular_acceleration;
        physics->angular_acceleration = 0.0f;

        float angular_speed = fabs(physics->angular_velocity);
        if (angular_speed < 0.1f) {
            physics->angular_velocity = 0.0f;
        } else if (angular_speed > physics->max_angular_speed) {
            physics->angular_velocity = physics->max_angular_speed * sign(physics->angular_velocity);
        }

        // move up?
        if (physics->collision.entities->size > 0) {
            physics->angular_velocity = 0.0f;
        }
        
        List_clear(physics->collision.entities);
        physics->collision.overlap = zeros();
        physics->collision.velocity = zeros();

        physics->slowed = false;
        physics->on_ground = false;
    }
}


void draw_vectors(int camera) {
    for (int i = 0; i < game_data->components->entities; i++) {
        PhysicsComponent* physics = PhysicsComponent_get(i);
        if (!physics) continue;

        CoordinateComponent* coord = CoordinateComponent_get(i);
        if (!coord) continue;

        Vector2f pos = coord->position;
        Vector2f v = physics->velocity;
        Vector2f a = physics->acceleration;
        float angle = coord->angle;
        float angular_velocity = physics->angular_velocity;
        float angular_acceleration = physics->angular_acceleration;

        float line_width = 0.03f;
        draw_line(camera, pos, sum(pos, mult(0.1f, v)), line_width, COLOR_RED);
        draw_line(camera, pos, sum(pos, mult(0.1f, a)), line_width, COLOR_GREEN);
    }
}
