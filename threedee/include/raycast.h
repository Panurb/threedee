#pragma once

#include "component.h"
#include "grid.h"


typedef struct {
    Vector2 position;
    int entity;
    Vector2 normal;
    float distance;
} HitInfo;

HitInfo raycast(Vector2 start, Vector2 velocity, float range, ColliderGroup group);
