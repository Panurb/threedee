#include <stdlib.h>

#include "grid.h"

#include <component.h>
#include <scene.h>


void clear_grid(ColliderGrid* grid) {
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int z = 0; z < GRID_DEPTH; z++) {
                for (int i = 0; i < MAX_ENTITIES_PER_CELL; i++) {
                    grid->array[x][y][z][i] = NULL_ENTITY;
                }
            }
        }
    }
}


ColliderGrid* create_grid() {
    ColliderGrid* grid = malloc(sizeof(ColliderGrid));
    clear_grid(grid);
    return grid;
}


int clamp_bound(float center, int max) {
    int value = (int)floorf(center) + max / 2;
    value = mini(maxi(0, value), max - 1);
    return value;
}


Bounds get_bounds(Entity entity) {
    AABB bbox = get_bounding_box(entity);
    Bounds bounds = {
        .x_min = clamp_bound(bbox.center.x - bbox.half_extents.x, GRID_WIDTH),
        .y_min = clamp_bound(bbox.center.y - bbox.half_extents.y, GRID_HEIGHT),
        .z_min = clamp_bound(bbox.center.z - bbox.half_extents.z, GRID_DEPTH),
        .x_max = clamp_bound(bbox.center.x + bbox.half_extents.x, GRID_WIDTH),
        .y_max = clamp_bound(bbox.center.y + bbox.half_extents.y, GRID_HEIGHT),
        .z_max = clamp_bound(bbox.center.z + bbox.half_extents.z, GRID_DEPTH),
    };
    return bounds;
}


bool is_dynamic(Entity entity) {
    if (entity == NULL_ENTITY) {
        return false;
    }
    if (get_component(entity, COMPONENT_RIGIDBODY)) {
        return true;
    }
    return false;
}


void update_grid(ColliderGrid* grid) {
    clear_grid(grid);

    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        if (!get_component(entity, COMPONENT_COLLIDER)) {
            continue;
        }

        Bounds bounds = get_bounds(entity);

        for (int x = bounds.x_min; x <= bounds.x_max; x++) {
            for (int y = bounds.y_min; y <= bounds.y_max; y++) {
                for (int z = bounds.z_min; z <= bounds.z_max; z++) {
                    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT || z < 0 || z >= GRID_DEPTH) {
                        continue;
                    }

                    for (int i = 0; i < MAX_ENTITIES_PER_CELL; i++) {
                        if (grid->array[x][y][z][i] == NULL_ENTITY) {
                            grid->array[x][y][z][i] = entity;
                            break;
                        }
                    }
                }
            }
        }
    }
}
