#pragma once


#include <arraylist.h>
#include <util.h>


typedef enum {
    COLLIDER_PLANE,
    COLLIDER_SPHERE,
    COLLIDER_CUBE
} ColliderType;


typedef struct {
    Entity entity;
    Vector3 overlap;
    Vector3 offset;
    Vector3 offset_other;
} Collision;


typedef struct {
    ColliderType type;
    float radius;
    ArrayList* collisions;
} ColliderComponent;


void ColliderComponent_add(Entity entity, ColliderType type, float radius);

void ColliderComponent_remove(Entity entity);
