#pragma once

#include "component.h"
#include "grid.h"


typedef struct {
    Vector2f position;
    int entity;
    Vector2f normal;
    float distance;
} HitInfo;

HitInfo raycast(Vector2f start, Vector2f velocity, float range, ColliderGroup group);
