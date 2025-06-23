#pragma once


#include <arraylist.h>
#include <util.h>


typedef enum {
    COLLIDER_PLANE,
    COLLIDER_SPHERE,
    COLLIDER_CUBOID,
    COLLIDER_CAPSULE
} ColliderType;


typedef enum {
    GROUP_WALLS = 1 << 0,
    GROUP_PLAYERS = 1 << 1,
    GROUP_PROPS = 1 << 2,
} ColliderGroup;


typedef struct {
    Entity entity;
    Vector3 overlap;
    Vector3 offset;
    Vector3 offset_other;
} Collision;


typedef struct {
    ColliderType type;
    ColliderGroup group;
    float radius;
    float width;
    float height;
    float depth;
    ArrayList* collisions;
} ColliderComponent;


typedef struct {
    ColliderType type;
    ColliderGroup group;
    float radius;
    float width;
    float height;
    float depth;
} ColliderParameters;


float get_radius(Entity entity);

Vector3 get_half_extents(Entity entity);

Shape get_shape(Entity entity);

void ColliderComponent_add(Entity entity, ColliderParameters parameters);

void ColliderComponent_remove(Entity entity);
