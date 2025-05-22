#pragma once

#include "component.h"
#include "camera.h"
#include "list.h"


#define CHUNK_WIDTH 32
#define CHUNK_HEIGHT 32
#define LEVEL_WIDTH 13
#define LEVEL_HEIGHT 13
#define TILE_WIDTH 1.0f
#define TILE_HEIGHT 1.0f


typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} Bounds;

typedef struct {
    List* array[LEVEL_WIDTH * CHUNK_WIDTH][LEVEL_HEIGHT * CHUNK_HEIGHT];
    int columns;
    int rows;
    int tile_size;
    float tile_width;
    float tile_height;
    float width;
    float height;
} ColliderGrid;

ColliderGrid* ColliderGrid_create();

void ColliderGrid_clear(ColliderGrid* grid);

List* get_entities(Vector2 origin, float radius);

void update_grid(int i);

void init_grid();

void clear_grid(int i);

Bounds get_bounds(int i);

void get_neighbors(int i, int entities[100]);

void draw_grid(int camera, float tile_width, float tile_height);

Vector2 snap_to_grid(Vector2 vector, float tile_width, float tile_height);

Vector2 snap_to_grid_center(Vector2 vector, float tile_width, float tile_height);
