#pragma once

#include "util.h"


typedef struct {
    bool valid;
    Vector3 overlap;
    Vector3 contact_point;
} Penetration;


void update_collisions();
