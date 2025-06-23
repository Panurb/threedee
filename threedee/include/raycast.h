#pragma once
#include <util.h>


#include "components/collider.h"


typedef struct {
    float distance;
    Vector3 point;
    Vector3 normal;
} Intersection;

typedef struct {
    Vector3 origin;
    Vector3 direction;
} Ray;

typedef struct {
    Entity entity;
    float distance;
    Vector3 point;
    Vector3 normal;
} Hit;


Hit raycast(Ray ray, ColliderGroup group);
