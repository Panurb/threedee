#pragma once

#include "util.h"


typedef struct ContactManifold {
    Vector3 points[16];
    int points_size;
    Vector3 average_point;
} ContactManifold;


typedef struct {
    bool valid;
    Vector3 overlap;
    Vector3 contact_point;
    ContactManifold contact_manifold;
} Penetration;


void update_collisions();
