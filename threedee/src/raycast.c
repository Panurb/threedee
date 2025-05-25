#define _USE_MATH_DEFINES

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "component.h"
#include "util.h"
#include "collider.h"
#include "raycast.h"
#include "game.h"
#include "app.h"


typedef struct {
    float time;
    Vector2 normal;
} Hit;


Hit ray_intersection(int i, Vector2 start, Vector2 velocity, float range) {
    Hit hit = { range, perp(velocity) };

    ColliderComponent* col = ColliderComponent_get(i);

    if (!col->enabled) return hit;

    if (col->type == COLLIDER_RECTANGLE) {
        // https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282

        Vector2 corners[4];
        get_corners(i, corners);

        int n = 0;
        for (int k = 0; k < 4; k++) {
            Vector2 dir = diff(corners[(k + 1) % 4], corners[k]);
            float t = cross(diff(corners[k], start), dir) / cross(velocity, dir);
            float u = cross(diff(start, corners[k]), velocity) / cross(dir, velocity);

            // Aiming ray directly at corner can sometimes slip through so add some tolerance
            float delta = 0.001f;
            if (t >= 0.0 && u >= -delta && u <= 1.0f + delta) {
                n++;
                if (t < hit.time) {
                    hit.time = t;
                    hit.normal = perp(dir);
                }
            }
        }
    } else {
        // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

        float radius = col->radius;
        Vector2 oc = diff(start, get_position(i));
        float delta = powf(dot2(velocity, oc), 2) - norm2(oc) + powf(radius, 2);
        float t = -dot2(velocity, oc) - sqrtf(delta);

        if (delta >= 0.0 && t >= 0.0 && t < hit.time) {
            hit.time = t;
            Vector2 p = sum(start, mult(t, velocity));
            hit.normal = diff(p, get_position(i));
        }
    }

    return hit;
}


HitInfo raycast(Vector2 start, Vector2 velocity, float range, ColliderGroup group) {
    // http://www.cs.yorku.ca/~amana/research/grid.pdf

    static int id = MAX_ENTITIES;
    id = (id < 2 * MAX_ENTITIES) ? id + 1 : MAX_ENTITIES;

    HitInfo info;
    info.entity = -1;
    info.normal = mult(-1.0f, velocity);
    info.position = zeros2();
    info.distance = range;

    float v = norm(velocity);

    if (v == 0.0f) {
        LOG_WARNING("Raycast with zero velocity");
        return info;
    }

    velocity = mult(1.0f / v, velocity);

    // SDL_SetRenderTarget(app.renderer, NULL);

    ColliderGrid* grid = game_data->grid;

    // Starting ray at intersection of grid cells is undefined so nudge it slightly.
    start.x += 1e-3f * velocity.x;
    start.y += 1e-3f * velocity.y;

    int x = floorf((start.x + 0.5 * grid->width) / grid->tile_width);
    int y = floorf((start.y + 0.5 * grid->height) / grid->tile_height);

    int step_x = sign(velocity.x);
    int step_y = sign(velocity.y);

    float t_max_x = (grid->tile_width - mod(step_x * start.x, grid->tile_width)) / fabsf(velocity.x);
    float t_max_y = (grid->tile_height - mod(step_y * start.y, grid->tile_height)) / fabsf(velocity.y);

    float t_delta_x = grid->tile_width / fabs(velocity.x);
    float t_delta_y = grid->tile_height / fabs(velocity.y);
    
    float t_min = range;

    while (x > 0 && x < grid->columns && y > 0 && y < grid->rows) {
        // Vector2f v = vec((x + 0.5) * grid->tile_width - 0.5 * grid->width, (y + 0.5) * grid->tile_height - 0.5 * grid->height);
        // draw_rectangle(game_data->camera, v, grid->tile_width, grid->tile_height, 0.0f, get_color(0.0f, 0.0f, 1.0f, 0.1f));

        for (ListNode* current = grid->array[x][y]->head; current != NULL; current = current->next) {
            int j = current->value;
            if (j == -1) continue;

            ColliderComponent* col = ColliderComponent_get(j);
            if (col->last_collision == id) continue;
            if (COLLISION_MATRIX[group][col->group] == 0) continue;
            if (col->trigger_type != TRIGGER_NONE) continue;

            col->last_collision = id;

            // Skip if ray starts inside collider
            if (point_inside_collider(j, start)) {
                continue;
            }

            Hit hit = ray_intersection(j, start, velocity, range);
            if (hit.time < t_min) {
                t_min = hit.time;
                info.entity = j;
                info.normal = hit.normal;
                info.distance = t_min;
                range = t_min;
            }
        }

        if (fminf(t_max_x, t_max_y) > range) break;

        if (t_max_x < t_max_y) {
            t_max_x += t_delta_x;
            x += step_x;
        } else {
            t_max_y += t_delta_y;
            y += step_y;
        }
    }

    info.position = sum(start, mult(t_min, velocity));
    info.normal = normalized(info.normal);

    // draw_line(game_data->camera, start, sum(start, mult(range, velocity)), 0.01f, COLOR_WHITE);
    // draw_line(game_data->camera, start, info.position, 0.05f, COLOR_WHITE);

    // String buffer;
    // snprintf(buffer, STRING_SIZE, "x: %.2f, y: %.2f", start.y, grid->width);
    // // draw_text(game_data->camera, info.position, buffer, 20, COLOR_WHITE);

    // SDL_SetRenderTarget(app.renderer, app.light_texture);

    return info;
}
