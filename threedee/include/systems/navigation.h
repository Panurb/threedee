#pragma once

#include "camera.h"


#define MAX_NODES 1000
#define MAX_PATH_LENGTH 50


int create_waypoint(Vector3 pos);

bool a_star(int start, int goal, List* path);

void init_waypoints();

void update_waypoints();

void draw_waypoints(bool draw_neighbors);
