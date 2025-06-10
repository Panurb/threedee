#pragma once

#include "util.h"


typedef struct {
    bool valid;
    Vector3 overlap;
    Vector3 contact_point;
} Penetration;


float get_radius(Entity entity);

Vector3 get_half_extents(Entity entity);

void update_collisions();
