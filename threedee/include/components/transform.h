#pragma once

#include "linalg.h"
#include "util.h"
#include "list.h"


typedef struct {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    Entity parent;
    List* children;
    float lifetime;
    Filename prefab;
    struct {
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
    } previous;
} TransformComponent;


TransformComponent* TransformComponent_add(Entity entity, Vector3 pos);

void TransformComponent_remove(Entity entity);
