#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include "collider.h"
#include "component.h"
#include "camera.h"
#include "util.h"
#include "grid.h"
#include "physics.h"
#include "raycast.h"
#include "sound.h"
#include "particle.h"
#include "image.h"
#include "health.h"
#include "game.h"
#include "enemy.h"


float collider_width(int i) {
    Vector2f scale = get_scale(i);
    ColliderComponent* col = ColliderComponent_get(i);
    return col->width * scale.x;
}


float collider_height(int i) {
    Vector2f scale = get_scale(i);
    ColliderComponent* col = ColliderComponent_get(i);
    return col->height * scale.y;
}


float collider_radius(int i) {
    Vector2f scale = get_scale(i);
    ColliderComponent* col = ColliderComponent_get(i);
    if (col->type == COLLIDER_RECTANGLE) {
        return 0.5f * norm(vec(col->width * scale.x, col->height * scale.y));
    }

    if (!close_enough(scale.x, scale.y, 0.01f)) {
        LOG_WARNING("Circle collider %i has non-uniform scale\n", i);
    }
    return col->radius * scale.x;
}


bool point_inside_collider(int i, Vector2f point) {
    // https://math.stackexchange.com/questions/190111/how-to-check-if-a-point-is-inside-a-rectangle
    ColliderComponent* col = ColliderComponent_get(i);
    if (col->type == COLLIDER_RECTANGLE) {
        Vector2f pos = get_position(i);
        float angle = get_angle(i);
        float width = collider_width(i);
        float height = collider_height(i);
        return point_inside_rectangle(pos, angle, width, height, point);
    } else if (col->type == COLLIDER_CIRCLE) {
        float radius = collider_radius(i);
        if (norm2(diff(get_position(i), point)) < radius * radius) {
            return true;
        }
    }
    return false;
}


void get_corners(int i, Vector2f* corners) {
    Vector2f pos = get_position(i);

    Vector2f hw = half_width(i);
    Vector2f hh = half_height(i);

    corners[0] = sum(pos, sum(hw, hh));
    corners[1] = diff(corners[0], mult(2, hh));
    corners[2] = diff(corners[1], mult(2, hw));
    corners[3] = sum(corners[2], mult(2, hh));
}


Vector2f half_width(int i) {
    ColliderComponent* col = ColliderComponent_get(i);
    return polar_to_cartesian(0.5 * collider_width(i), get_angle(i));
}


Vector2f half_height(int i) {
    ColliderComponent* col = ColliderComponent_get(i);
    return polar_to_cartesian(0.5 * collider_height(i), get_angle(i) + 0.5 * M_PI);
}


float axis_half_width(int i, Vector2f axis) {
    ColliderComponent* col = ColliderComponent_get(i);

    if (col->type == COLLIDER_RECTANGLE) {
        Vector2f hw = half_width(i);
        Vector2f hh = half_height(i);
        return fabs(dot(hw, axis)) + fabs(dot(hh, axis));
    } else {
        return collider_radius(i);
    }
    return 0.0;
}


float axis_overlap(float w1, Vector2f r1, float w2, Vector2f r2, Vector2f axis) {
    float r = dot(diff(r1, r2), axis);
    float o = w1 + w2 - fabs(r);
    if (o > 0.0) {
        if (fabs(r) < 1e-6) {
            return o;
        } else {
            return copysignf(o, r);
        }
    }

    return 0.0;
}


Vector2f overlap_circle_circle(int i, int j) {
    Vector2f a = get_position(i);
    Vector2f b = get_position(j);

    float d = dist(a, b);
    float r = collider_radius(i) + collider_radius(j);

    if (d > r) {
        return (Vector2f) { 0.0, 0.0 };
    }

    if (d == 0.0) {
        return (Vector2f) { 0.0, r };
    }

    return mult(r / d - 1, diff(a, b));
}


Vector2f overlap_rectangle_circle(int i, int j) {
    bool near_corner = true;

    float overlaps[2] = { 0.0, 0.0 };
    Vector2f axes[2];
    axes[0] = polar_to_cartesian(1.0, get_angle(i));
    axes[1] = perp(axes[0]);

    Vector2f a = get_position(i);
    Vector2f b = get_position(j);
    float radius = ColliderComponent_get(j)->radius;

    for (int k = 0; k < 2; k++) {
        overlaps[k] = axis_overlap(axis_half_width(i, axes[k]), a, radius, b, axes[k]);

        if (fabs(overlaps[k]) < 1e-6) {
            return (Vector2f) { 0.0, 0.0 };
        }

        if (fabs(overlaps[k]) >= radius) {
            near_corner = false;
        }
    }

    int k = abs_argmin(overlaps, 2);
    if (!near_corner) {
        return mult(overlaps[k], axes[k]);
    }

    Vector2f hw = half_width(i);
    Vector2f hh = half_height(i);

    Vector2f corner = diff(a, sum(mult(sign(overlaps[0]), hw), mult(sign(overlaps[1]), hh)));

    Vector2f axis = normalized(diff(corner, b));

    float overlap = axis_overlap(axis_half_width(i, axis), a, radius, b, axis);

    if (0.0 < fabs(overlap) && fabs(overlap) < fabs(overlaps[k])) {
        return mult(overlap, axis);
    }

    return (Vector2f) { 0.0, 0.0 };
}

Vector2f overlap_circle_rectangle(int i, int j) {
    return mult(-1.0, overlap_rectangle_circle(j, i));
}

Vector2f overlap_rectangle_rectangle(int i, int j) {
    Vector2f hw_i = polar_to_cartesian(1.0, get_angle(i));
    Vector2f hw_j = polar_to_cartesian(1.0, get_angle(j));

    float overlaps[4];
    Vector2f axes[4] = { hw_i, perp(hw_i), hw_j, perp(hw_j) };

    Vector2f a = get_position(i);
    Vector2f b = get_position(j);

    for (int k = 0; k < 4; k++) {
        overlaps[k] = axis_overlap(axis_half_width(i, axes[k]), a,
                                   axis_half_width(j, axes[k]), b, axes[k]);

        if (fabs(overlaps[k]) < 1e-6) {
            return (Vector2f) { 0.0, 0.0 };
        }
    }

    int k = abs_argmin(overlaps, 4);

    return mult(overlaps[k], axes[k]);
}


Vector2f overlap_collider_collider(int i, int j) {
    Vector2f ol = zeros();

    ColliderComponent* a = ColliderComponent_get(i);
    ColliderComponent* b = ColliderComponent_get(j);

    if (!a->enabled || !b->enabled) {
        return ol;
    }

    if (a->type == COLLIDER_CIRCLE) {
        if (b->type == COLLIDER_CIRCLE) {
            ol = overlap_circle_circle(i, j);
        } else {
            ol = overlap_circle_rectangle(i, j);
        }
    } else {
        if (b->type == COLLIDER_CIRCLE) {
            ol = overlap_rectangle_circle(i, j);
        } else {
            ol = overlap_rectangle_rectangle(i, j);
        }
    }

    return ol;
}


Vector2f overlap_rectangle_image(int i, int j) {
    ImageComponent* image = ImageComponent_get(j);

    Vector2f hw_i = polar_to_cartesian(1.0, get_angle(i));
    Vector2f hw_j = polar_to_cartesian(1.0, get_angle(j));

    float overlaps[4] = { 0.0, 0.0, 0.0, 0.0 };
    Vector2f axes[4] = { hw_i, perp(hw_i), hw_j, perp(hw_j) };

    Vector2f a = get_position(i);
    Vector2f b = get_position(j);

    for (int k = 0; k < 4; k++) {
        Vector2f hw = polar_to_cartesian(0.5 * image_width(j), get_angle(j));
        Vector2f hh = polar_to_cartesian(0.5 * image_height(j), get_angle(j) + 0.5 * M_PI);
        float image_axis_half_width = fabs(dot(hw, axes[k])) + fabs(dot(hh, axes[k]));

        overlaps[k] = axis_overlap(axis_half_width(i, axes[k]), a,
                                   image_axis_half_width, b, axes[k]);

        if (fabs(overlaps[k]) < 1e-6) {
            return zeros();
        }
    }

    int k = abs_argmin(overlaps, 4);

    return mult(overlaps[k], axes[k]);
}


bool collides_with(int i, List* entities) {
    ColliderGroup group = ColliderComponent_get(i)->group;
    Bounds bounds = get_bounds(i);

    for (int j = bounds.left; j <= bounds.right; j++) {
        for (int k = bounds.bottom; k <= bounds.top; k++) {
            for (ListNode* current = game_data->grid->array[j][k]->head; current != NULL; current = current->next) {
                int n = current->value;
                if (n == i) continue;

                ColliderComponent* collider = ColliderComponent_get(n);

                if (!COLLISION_MATRIX[group][collider->group]) {
                    continue;
                }

                if (collider->last_collision == i) {
                    continue;
                }
                collider->last_collision = i;

                Vector2f ol = overlap_collider_collider(i, n);

                if (non_zero(ol)) {
                    if (entities) {
                        List_add(entities, n);
                    } else {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


void apply_trigger(int trigger, int target) {
    CoordinateComponent* coord = CoordinateComponent_get(trigger);
    ColliderComponent* collider = ColliderComponent_get(trigger);
    PlayerComponent* player = PlayerComponent_get(target);
    PhysicsComponent* physics = PhysicsComponent_get(target);
    HealthComponent* health = HealthComponent_get(target);

    switch (collider->trigger_type) {
        case TRIGGER_NONE:
            break;
    }
}


void collide(int entity) {
    ColliderComponent* collider = ColliderComponent_get(entity);
    if (!collider) return;

    PhysicsComponent* physics = PhysicsComponent_get(entity);
    if (!physics) return;

    Bounds bounds = get_bounds(entity);

    for (int j = bounds.left; j <= bounds.right; j++) {
        for (int k = bounds.bottom; k <= bounds.top; k++) {
            for (ListNode* current = game_data->grid->array[j][k]->head; current; current = current->next) {
                int n = current->value;
                if (n == entity) continue;
                ColliderComponent* collider_other = ColliderComponent_get(n);
                if (collider_other->last_collision == entity) continue;

                collider_other->last_collision = entity;

                int collision_type = COLLISION_MATRIX[collider->group][collider_other->group];

                if (collision_type == 0) continue;

                Vector2f ol = overlap_collider_collider(entity, n);

                if (!non_zero(ol)) continue;

                if (collider_other->trigger_type != TRIGGER_NONE) {
                    apply_trigger(n, entity);

                    // Entity died by hurt trigger
                    if (!ColliderComponent_get(entity)) {
                        return;
                    }
                    continue;
                }

                Vector2f dv = physics->velocity;
                Vector2f no = normalized(ol);

                float m = 1.0f;

                PhysicsComponent* physics_other = PhysicsComponent_get(n);
                if (physics_other) {
                    dv = diff(dv, physics_other->velocity);
                    m = physics_other->mass / (physics->mass + physics_other->mass);
                }

                Vector2f new_vel = diff(physics->velocity, mult(2.0f * m * dot(dv, no), no));

                switch (collision_type) {
                    case 0:
                        break;
                    case 1:
                        physics->collision.velocity = sum(physics->collision.velocity, new_vel);
                        physics->collision.overlap = sum(physics->collision.overlap, mult(m, ol));
                        List_add(physics->collision.entities, n);
                        break;
                    case 2:
                        apply_force(entity, mult(fminf(50.0f * norm(ol), 50.0f), normalized(ol)));

                        ImageComponent* image = ImageComponent_get(n);
                        if (image) {
                            float x = norm(proj(ol, half_width(n)));
                            float y = norm(proj(ol, half_height(n)));
                            image->stretch = x / collider_width(n) - y / collider_height(n);
                            image->stretch *= 0.5f;
                            image->stretch_speed = 0.0f;
                        }
                        break;
                    case 3:
                        if (VehicleComponent_get(entity)) {
                            VehicleComponent_get(entity)->on_road = true;
                        }
                        break;
                }
            }
        }
    }
}


void update_collisions() {
    // https://en.wikipedia.org/wiki/Elastic_collision#Two-dimensional

    for (int i = 0; i < game_data->components->entities; i++) {
        ColliderComponent* collider = ColliderComponent_get(i);
        if (!collider) continue;

        collider->last_collision = -1;
    }

    for (int i = 0; i < game_data->components->entities; i++) {
        collide(i);
    }
}


void draw_occupied_tiles(int camera) {
    ColliderGrid* grid = game_data->grid;
    Vector2f w = { 0.5f * grid->tile_width, 0.0f };
    Vector2f h = { 0.0f, 0.5f * grid->tile_height };
    float linewidth = 0.01f;
    Color color = get_color(1.0f, 1.0f, 1.0f, 0.5f);
    for (int i = 0; i < grid->width; i++) {
        for (int j = 0; j < grid->height; j++) {
            Vector2f r = { (i + 0.5f) * grid->tile_width - 0.5f * grid->width, (j + 0.5f) * grid->tile_height - 0.5f * grid->height };
            if (on_screen(camera, r, grid->tile_width, grid->tile_height)) {
                draw_line(camera, sum(sum(r, w), h), sum(diff(r, w), h), linewidth, color);
                draw_line(camera, sum(diff(r, w), h), diff(diff(r, w), h), linewidth, color);
                draw_line(camera, diff(diff(r, w), h), diff(sum(r, w), h), linewidth, color);
                draw_line(camera, diff(sum(r, w), h), sum(sum(r, w), h), linewidth, color);
                if (grid->array[i][j]->size > 0) {
                    char size[10];
                    snprintf(size, 10, "%i", grid->array[i][j]->size);
                    draw_text(camera, r, size, 20, COLOR_WHITE);
                }
            }
        }
    }
}


void draw_colliders(int camera) {
    for (int i = 0; i < game_data->components->entities; i++) {
        ColliderComponent* col = game_data->components->collider[i];
        if (!col) continue;

        Vector2f pos = get_position(i);
        if (col->type == COLLIDER_CIRCLE) {
            draw_circle(camera, pos, collider_radius(i), get_color(1.0, 0.0, 1.0, 0.25));
            draw_line(camera, pos, sum(pos, half_width(i)), 0.05f, COLOR_WHITE);
        } else {
            Color color = get_color(0.0, 1.0, 1.0, 0.25);
            float width = collider_width(i);
            float height = collider_height(i);
            draw_rectangle(camera, get_position(i), width, height, get_angle(i), color);
        }
    }
}
