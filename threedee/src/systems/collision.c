#include "systems/collision.h"

#include <math.h>
#include <stdio.h>

#include "scene.h"
#include "util.h"


float get_radius(Entity entity) {
    // Use y component since this works nicely with plane offset.
    float scale = get_scale(entity).y;
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (collider) {
        return collider->radius * scale;
    }
    LOG_WARNING("No collider for entity %d", entity);
    return 0.0f;
}


Vector3 overlap_sphere_sphere(Vector3 pos1, float radius1, Vector3 pos2, float radius2) {
    Vector3 diff = diff3(pos1, pos2);
    float dist_sq = dot3(diff, diff);
    float radius_sum = radius1 + radius2;
    if (dist_sq < radius_sum * radius_sum) {
        float dist = sqrtf(dist_sq);
        if (dist == 0.0f) {
            return zeros3();
        }
        return mult3((radius_sum - dist) / dist, diff);
    }
    return zeros3();
}


Vector3 overlap_sphere_plane(Vector3 pos, float radius, Vector3 plane_normal, float plane_offset) {
    float dist = dot3(pos, plane_normal) - plane_offset;
    if (dist < radius) {
        return mult3((radius - dist) / norm3(plane_normal), plane_normal);
    }
    return zeros3();
}


Vector3 get_overlap(Entity i, Entity j) {
    ColliderComponent* collider = scene->components->collider[i];
    ColliderComponent* other_collider = scene->components->collider[j];

    Vector3 position = get_position(i);
    Vector3 other_position = get_position(j);

    if (!collider || !other_collider) {
        return zeros3();
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_SPHERE) {
        return overlap_sphere_sphere(position, get_radius(i), other_position, get_radius(j));
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_PLANE) {
        Vector4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
        Matrix4 transform = get_transform(j);
        up = matrix4_map(transform, up);
        Vector3 plane_normal = (Vector3) { up.x, up.y, up.z };
        float plane_offset = dot3(other_position, plane_normal);

        return overlap_sphere_plane(position, get_radius(i) + get_radius(j), plane_normal, plane_offset);
    }

    if (collider->type == COLLIDER_PLANE && other_collider->type == COLLIDER_SPHERE) {
        return get_overlap(j, i);
    }

    // LOG_ERROR("Invalid collision: %d vs %d", collider->type, other_collider->type);

    return zeros3();
}


void update_collisions() {
    for (Entity i = 0; i < scene->components->entities; i++) {
        ColliderComponent* collider = get_component(i, COMPONENT_COLLIDER);
        if (!collider) continue;

        ArrayList_clear(collider->collisions);
    }

    for (Entity i = 0; i < scene->components->entities; i++) {
        ColliderComponent* collider = get_component(i, COMPONENT_COLLIDER);
        if (!collider) continue;

        for (Entity j = 0; j < scene->components->entities; j++) {
            if (i == j) continue;
            ColliderComponent* other_collider = get_component(j, COMPONENT_COLLIDER);
            if (!other_collider) continue;

            Vector3 overlap = get_overlap(i, j);
            if (non_zero3(overlap)) {
                Collision collision = {
                    .entity = j,
                    .overlap = overlap
                };
                ArrayList_add(collider->collisions, &collision);
            }
        }
    }
}
