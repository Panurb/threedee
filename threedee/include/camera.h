#pragma once

#include <SDL3_image/SDL_image.h>

#include "component.h"


typedef enum {
    SHADER_NONE,
    SHADER_OUTLINE
} CameraShader;

int create_camera();

int create_menu_camera();

Vector2 camera_size(int camera);

Vector3 look_direction(Entity camera);

Vector3 world_to_screen(int camera, Vector3 a);

Vector3 screen_to_world(int camera, Vector3 a);
