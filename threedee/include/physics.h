#pragma once

#include "component.h"


void apply_force(int entity, Vector2f force);

void update_physics(float time_step);

void draw_vectors(int camera);
