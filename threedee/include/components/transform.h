#pragma once

#include "linalg.h"
#include "util.h"
#include "list.h"


typedef struct TransformParameters {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    Entity parent;
    float yaw;
} TransformParameters;


typedef struct {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    Entity parent;
    List* children;
    float lifetime;
    String prefab;
    struct {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
    } previous;
} TransformComponent;


TransformComponent* TransformComponent_add(Entity entity, TransformParameters params);

void TransformComponent_remove(Entity entity);
