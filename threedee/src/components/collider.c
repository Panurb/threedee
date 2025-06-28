#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>

#include "components/collider.h"

#include <render.h>
#include <stdio.h>

#include "scene.h"
#include "util.h"


float get_radius(Entity entity) {
    Vector3 scale = get_scale(entity);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (collider) {
        // if (collider->type == COLLIDER_SPHERE) {
        //     if (scale.x != scale.y || scale.x != scale.z) {
        //         LOG_WARNING("Sphere collider with non-uniform scale for entity %d", entity);
        //     }
        // }
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
        return (Vector3) {
            collider->width * scale.x / 2.0f,
            collider->height * scale.y / 2.0f,
            collider->depth * scale.z / 2.0f
        };
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
        case COLLIDER_AABB:
            shape.aabb = (AABB) {
                .center = position,
                .half_extents = get_half_extents(entity),
            };
            break;
    }

    return shape;
}


void ColliderComponent_add(Entity entity, ColliderParameters parameters) {
    ColliderComponent* collider = malloc(sizeof(ColliderComponent));
    collider->type = parameters.type;
    collider->group = parameters.group ? parameters.group : GROUP_WALLS;

    switch (collider->type) {
        case COLLIDER_PLANE:
            collider->radius = parameters.height;
            collider->width = INFINITY;
            collider->height = parameters.height;
            collider->depth = INFINITY;
            break;
        case COLLIDER_SPHERE:
            collider->radius = parameters.radius ? parameters.radius : 1.0f;
            collider->width = 2.0f * collider->radius;
            collider->height = 2.0f * collider->radius;
            collider->depth = 2.0f * collider->radius;
            break;
        case COLLIDER_CUBOID:
            collider->width = parameters.width ? parameters.width : 1.0f;
            collider->height = parameters.height ? parameters.height : 1.0f;
            collider->depth = parameters.depth ? parameters.depth : 1.0f;
            collider->radius = norm3(vec3(collider->width, collider->height, collider->depth)) / 2.0f;
            break;
        case COLLIDER_CAPSULE:
            collider->radius = parameters.radius ? parameters.radius : 0.5f;
            collider->width = 2.0f * collider->radius;
            collider->height = parameters.height ? parameters.height : 1.0f;
            collider->depth = 2.0f * collider->width;
            break;
        case COLLIDER_AABB:
            collider->width = parameters.width ? parameters.width : 1.0f;
            collider->height = parameters.height ? parameters.height : 1.0f;
            collider->depth = parameters.depth ? parameters.depth : 1.0f;
            collider->radius = norm3(vec3(collider->width, collider->height, collider->depth)) / 2.0f;
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


void draw_collider(Entity entity) {
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (!collider) return;

    Vector3 position = get_position(entity);
    Vector3 scale = get_scale(entity);
    Matrix3 rot = quaternion_to_rotation_matrix(get_rotation(entity));

    Color color = get_color(1.0f, 0.0f, 0.0f, 0.5f);

    switch (collider->type) {
        case COLLIDER_PLANE: {
            render_plane(get_shape(entity).plane, color);
            break;
        }
        case COLLIDER_SPHERE:
            render_circle(position, collider->radius * scale.x, 32, color);
            break;
        case COLLIDER_CUBOID:
            break;
        case COLLIDER_CAPSULE:
            Vector3 h = matrix3_map(rot, vec3(0.0f, collider->height / 2.0f, 0.0f));
            Vector3 p0 = sum3(position, h);
            Vector3 p1 = sum3(position, neg3(h));
            render_sphere(p0, collider->radius, 16, color);
            render_sphere(p1, collider->radius, 16, color);
            break;
        case COLLIDER_AABB:
            break;
    }
}
