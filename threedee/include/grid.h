#pragma once

#include "util.h"


#define GRID_WIDTH 20
#define GRID_HEIGHT 10
#define GRID_DEPTH 20
#define CELL_WIDTH 1
#define CELL_HEIGHT 1
#define CELL_DEPTH 1
#define MAX_ENTITIES_PER_CELL 10


typedef struct ColliderGrid {
    Entity array[GRID_WIDTH][GRID_HEIGHT][GRID_DEPTH][MAX_ENTITIES_PER_CELL];
} ColliderGrid;


typedef struct Bounds {
    int x_min;
    int y_min;
    int z_min;
    int x_max;
    int y_max;
    int z_max;
} Bounds;


ColliderGrid* create_grid();

Bounds get_bounds(Entity entity);

void update_grid(ColliderGrid* grid);
