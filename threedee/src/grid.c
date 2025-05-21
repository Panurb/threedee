#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#include "grid.h"
#include "component.h"
#include "collider.h"
#include "util.h"
#include "camera.h"
#include "list.h"
#include "game.h"


ColliderGrid* ColliderGrid_create() {
    ColliderGrid* grid = malloc(sizeof(ColliderGrid));
    grid->columns = LEVEL_WIDTH * CHUNK_WIDTH;
    grid->rows = LEVEL_HEIGHT * CHUNK_HEIGHT;
    grid->tile_size = 10;
    grid->tile_width = TILE_WIDTH;
    grid->tile_height = TILE_HEIGHT;
    grid->width = grid->columns * grid->tile_width;
    grid->height = grid->rows * grid->tile_height;

    for (int i = 0; i < grid->columns; i++) {
        for (int j = 0; j < grid->rows; j++) {
            grid->array[i][j] = List_create();
        }
    }

    return grid;
}


void ColliderGrid_clear(ColliderGrid* grid) {
    for (int i = 0; i < grid->columns; i++) {
        for (int j = 0; j < grid->rows; j++) {
            List_clear(grid->array[i][j]);
        }
    }
}


Bounds get_bounds(int i) {
    Vector2f pos = get_position(i);
    
    float x = (pos.x + 0.5 * game_data->grid->width) / game_data->grid->tile_width;
    float y = (pos.y + 0.5 * game_data->grid->height) / game_data->grid->tile_height;

    float w = axis_half_width(i, (Vector2f) { 1.0, 0.0 }) / game_data->grid->tile_width;
    float h = axis_half_width(i, (Vector2f) { 0.0, 1.0 }) / game_data->grid->tile_height;

    Bounds bounds;
    bounds.left = maxi(0, floorf(x - w));
    bounds.right = mini(game_data->grid->columns - 1, floorf(x + w));
    bounds.bottom = maxi(0, floorf(y - h));
    bounds.top = mini(game_data->grid->rows - 1, floorf(y + h));

    return bounds;
}


List* get_entities(Vector2f origin, float radius) {
    List* list = List_create();

    static int id = 2 * MAX_ENTITIES;
    id = (id < 3 * MAX_ENTITIES) ? id + 1 : 2 * MAX_ENTITIES;

    float x = (origin.x + 0.5f * game_data->grid->width) / game_data->grid->tile_width;
    float y = (origin.y + 0.5f * game_data->grid->height) / game_data->grid->tile_height;
    float r = radius / game_data->grid->tile_width;
      
    for (int i = maxi(0, x - r); i <= mini(game_data->grid->columns - 1, x + r); i++) {
        for (int j = maxi(0, y - r); j <= mini(game_data->grid->rows - 1, y + r); j++) {
            for (ListNode* node = game_data->grid->array[i][j]->head; node; node = node->next) {
                int n = node->value;

                ColliderComponent* col = ColliderComponent_get(n);
                if (col->last_collision == id) continue;

                if (dist(origin, get_position(n)) <= radius + collider_radius(n)) {
                    List_add(list, n);
                }

                col->last_collision = id;
            }
        }
    }

    return list;
}


void update_grid(int entity) {
    Bounds bounds = get_bounds(entity);

    for (int i = bounds.left; i <= bounds.right; i++) {
        for (int j = bounds.bottom; j <= bounds.top; j++) {
            List_add(game_data->grid->array[i][j], entity);
        }
    }
}


void init_grid() {
    for (int i = 0; i < game_data->components->entities; i++) {
        ColliderComponent* collider = ColliderComponent_get(i);
        if (collider && collider->enabled) {
            update_grid(i);
        }
    }
}


void clear_grid(int entity) {
    Bounds bounds = get_bounds(entity);
    
    for (int i = bounds.left; i <= bounds.right; i++) {
        for (int j = bounds.bottom; j <= bounds.top; j++) {
            List_remove(game_data->grid->array[i][j], entity);
        }
    }
}


void get_neighbors(int entity, int entities[100]) {
    int id = 2 * MAX_ENTITIES + 2;

    Bounds bounds = get_bounds(entity);

    int l = 0;
    for (int i = bounds.left; i <= bounds.right; i++) {
        for (int j = bounds.bottom; j <= bounds.top; j++) {
            for (ListNode* current = game_data->grid->array[i][j]->head; current != NULL; current = current->next) {
                int n = current->value;
                if (n == -1 || n == entity) continue;

                ColliderComponent* col = ColliderComponent_get(n);
                if (col->last_collision == id) continue;

                entities[l] = n;
                l++;

                col->last_collision = id;
            }
        }
    }
}


void draw_grid(int camera, float tile_width, float tile_height) {
    CameraComponent* cam = CameraComponent_get(camera);
    float width = cam->resolution.w / cam->zoom;
    float height = cam->resolution.h / cam->zoom;
    Vector2f pos = get_position(camera);
    float major_lines = 16.0f;
    float left = pos.x - 0.5f * width;
    float right = pos.x + 0.5f * width;
    float bottom = pos.y - 0.5f * height;
    float top = pos.y + 0.5f * height;
    Color color = get_color(1.0f, 1.0f, 1.0f, 0.1f);
    
    for (float x = left - mod(left, tile_width); x < right; x += tile_width) {
        float linewidth = mod(x, major_lines) == 0.0f ? 0.2f : 0.01f;
        draw_line(camera, vec(x, bottom), vec(x, top), linewidth, color);
    }

    for (float y = bottom - mod(bottom, tile_height); y < top; y += tile_height) {
        float linewidth = mod(y, major_lines) == 0.0f ? 0.2f : 0.01f;
        draw_line(camera, vec(left, y), vec(right, y), linewidth, color);
    }
}


Vector2f snap_to_grid(Vector2f vector, float tile_width, float tile_height) {
    vector.x = tile_width * roundf(vector.x / tile_width);
    vector.y = tile_height * roundf(vector.y / tile_height);
    return vector;
}


Vector2f snap_to_grid_center(Vector2f vector, float tile_width, float tile_height) {
    vector.x = tile_width * roundf((vector.x + 0.5f) / tile_width) - 0.5f * tile_width;
    vector.y = tile_height * roundf((vector.y + 0.5f) / tile_height) - 0.5f * tile_height;
    return vector;
}
