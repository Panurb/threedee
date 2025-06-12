#pragma once


#include "util.h"


typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    Resolution resolution;
    Matrix4 projection_matrix;
} CameraComponent;


CameraComponent* CameraComponent_add(Entity entity, Resolution resolution, float fov);

void CameraComponent_remove(Entity entity);
