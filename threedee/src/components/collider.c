#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>

#include "components/collider.h"

#include <stdio.h>

#include "scene.h"
#include "util.h"


float get_radius(Entity entity) {
    Vector3 scale = get_scale(entity);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (collider) {
        if (collider->type == COLLIDER_SPHERE) {
            if (scale.x != scale.y || scale.x != scale.z) {
                LOG_WARNING("Sphere collider with non-uniform scale for entity %d", entity);
            }
        }
        // Use y component since this works nicely with plane offset.
        return collider->radius * scale.y;
    }
    LOG_WARNING("No collider for entity %d", entity);
    return 0.0f;
}


Vector3 get_half_extents(Entity entity) {
    Vector3 scale = get_scale(entity);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (collider) {
        return mult3(collider->radius, scale);
    }
    LOG_WARNING("No collider for entity %d", entity);
    return zeros3();
}


Shape get_shape(Entity entity) {
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (!collider) {
        LOG_WARNING("No collider for entity %d", entity);
        return (Shape) { 0 };
    }

    Vector3 position = get_position(entity);
    float radius = get_radius(entity);
    Shape shape = { 0 };

    switch (collider->type) {
        case COLLIDER_PLANE: {
            Vector4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
            Matrix4 transform = get_transform(entity);
            up = matrix4_map(transform, up);
            Vector3 plane_normal = (Vector3) { up.x, up.y, up.z };
            float plane_offset = dot3(position, plane_normal) + radius;

            shape.plane = (Plane) {
                .normal = plane_normal,
                .offset = plane_offset
            };
            break;
        }
        case COLLIDER_SPHERE:
            shape.sphere = (Sphere) {
                .center = position,
                .radius = radius
            };
            break;
        case COLLIDER_CUBOID:
            shape.cuboid = (Cuboid) {
                .center = position,
                .half_extents = get_half_extents(entity),
                .rotation = get_rotation(entity),
            };
            break;
        case COLLIDER_CAPSULE:
            shape.capsule = (Capsule) {
                .center = position,
                .radius = radius,
                .height = get_scale(entity).y * collider->height,
                .rotation = get_rotation(entity),
            };
            break;
    }

    return shape;
}


void ColliderComponent_add(Entity entity, ColliderParameters parameters) {
    ColliderComponent* collider = malloc(sizeof(ColliderComponent));
    collider->type = parameters.type;

    switch (collider->type) {
        case COLLIDER_PLANE:
            collider->radius = parameters.height;
            collider->width = INFINITY;
            collider->height = parameters.height;
            collider->depth = INFINITY;
            break;
        case COLLIDER_SPHERE:
            collider->radius = parameters.radius;
            collider->width = 2.0f * parameters.radius;
            collider->height = 2.0f * parameters.radius;
            collider->depth = 2.0f * parameters.radius;
            break;
        case COLLIDER_CUBOID:
            collider->radius = sqrtf(
                parameters.width * parameters.width + parameters.height * parameters.height
                + parameters.depth * parameters.depth) / 2.0f;
            collider->width = parameters.width;
            collider->height = parameters.height;
            collider->depth = parameters.depth;
            break;
        case COLLIDER_CAPSULE:
            collider->radius = parameters.radius;
            collider->width = 2.0f * parameters.radius;
            collider->height = parameters.height;
            collider->depth = collider->width;
            break;
    }

    collider->collisions = ArrayList_create(sizeof(Collision));
    scene->components->collider[entity] = collider;
}


void ColliderComponent_remove(Entity entity) {
    ColliderComponent* collider = scene->components->collider[entity];
    if (collider) {
        ArrayList_destroy(collider->collisions);
        free(collider);
        scene->components->collider[entity] = NULL;
    }
}
