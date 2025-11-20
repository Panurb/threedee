#pragma once

#include "linalg.h"
#include "util.h"


typedef struct CubeFace {
    Vector3 normal;
    Vector3 corners[4];
    Entity entity;
} CubeFace;


void merge_adjacent_faces(void);

void draw_entities(void);
