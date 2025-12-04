#pragma once

#include "component.h"


int create_camera();

int create_screen_camera();

Entity create_overhead_camera();

Vector2 camera_size(int camera);

Vector3 look_direction(Entity camera);

Vector3 world_to_screen(int camera, Vector3 a);

Vector3 screen_to_world(int camera, Vector3 a);

Frustum get_camera_frustum(Entity entity, float delta);
