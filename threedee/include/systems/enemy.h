#pragma once


#include "util.h"


Entity create_enemy(Vector3 pos, float yaw);

void update_enemies(float time_step);

void debug_draw_enemies();

void create_window_scare(Vector3 position, float yaw);
