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


Vector3 get_half_extents(Entity entity) {
    Vector3 scale = get_scale(entity);
    ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
    if (collider) {
        return mult3(collider->radius, scale);
    }
    LOG_WARNING("No collider for entity %d", entity);
    return zeros3();
}


Penetration overlap_sphere_sphere(Vector3 pos1, float radius1, Vector3 pos2, float radius2) {
    Penetration penetration = {
        .valid = false
    };

    Vector3 diff = diff3(pos1, pos2);
    float dist_sq = dot3(diff, diff);
    float radius_sum = radius1 + radius2;
    if (dist_sq < radius_sum * radius_sum) {
        float dist = sqrtf(dist_sq);
        if (dist == 0.0f) {
            return penetration;
        }
        penetration.valid = true;
        penetration.overlap = mult3((radius_sum - dist) / dist, diff);
        penetration.contact_point = sum3(pos1, mult3(radius1 / (radius1 + radius2), diff));
    }

    return penetration;
}


Penetration overlap_sphere_plane(Vector3 pos, float radius, Vector3 plane_normal, float plane_offset) {
    Penetration penetration = {
        .valid = false
    };
    float dist = dot3(pos, plane_normal) - plane_offset;
    if (dist < radius) {
        penetration.valid = true;
        penetration.overlap = mult3(radius - dist, plane_normal);
        penetration.contact_point = diff3(pos, mult3(radius, plane_normal));
    }
    return penetration;
}


Penetration overlap_cuboid_plane(Cuboid cuboid, Plane plane) {
    Matrix3 rot = quaternion_to_rotation_matrix(cuboid.rotation);

    Vector3 vertices[8] = {
        { -cuboid.half_size.x, -cuboid.half_size.y, -cuboid.half_size.z },
        {  cuboid.half_size.x, -cuboid.half_size.y, -cuboid.half_size.z },
        {  cuboid.half_size.x,  cuboid.half_size.y, -cuboid.half_size.z },
        { -cuboid.half_size.x,  cuboid.half_size.y, -cuboid.half_size.z },
        { -cuboid.half_size.x, -cuboid.half_size.y,  cuboid.half_size.z },
        {  cuboid.half_size.x, -cuboid.half_size.y,  cuboid.half_size.z },
        {  cuboid.half_size.x,  cuboid.half_size.y,  cuboid.half_size.z },
        { -cuboid.half_size.x,  cuboid.half_size.y,  cuboid.half_size.z }
    };

    Penetration penetration = {
        .valid = false,
        .contact_point = zeros3(),
    };

    float min_dist = 0.0f;
    int penetration_count = 0;
    for (int i = 0; i < 8; i++) {
        Vector3 v = sum3(cuboid.center, matrix3_map(rot, vertices[i]));
        float dist = dot3(v, plane.normal) - plane.offset;
        if (dist < 0.0f) {
            min_dist = fminf(min_dist, dist);
            penetration_count++;
            penetration.contact_point = sum3(penetration.contact_point, v);
        }
    }

    if (penetration_count > 0) {
        penetration.valid = true;
        penetration.contact_point = mult3(1.0f / (float)penetration_count, penetration.contact_point);
        penetration.overlap = mult3(-min_dist, plane.normal);
    }

    return penetration;
}


Penetration get_overlap(Entity i, Entity j) {
    ColliderComponent* collider = scene->components->collider[i];
    ColliderComponent* other_collider = scene->components->collider[j];

    if (!collider || !other_collider) {
        return (Penetration) {
            .valid = false
        };
    }

    Vector3 position = get_position(i);
    Vector3 other_position = get_position(j);

    Shape shape;
    Shape shape_other = { 0 };

    if (other_collider->type == COLLIDER_PLANE) {
        Vector4 up = { 0.0f, 1.0f, 0.0f, 0.0f };
        Matrix4 transform = get_transform(j);
        up = matrix4_map(transform, up);
        Vector3 plane_normal = (Vector3) { up.x, up.y, up.z };
        float plane_offset = dot3(other_position, plane_normal) + get_radius(j);

        shape_other.plane = (Plane) {
            .normal = plane_normal,
            .offset = plane_offset
        };
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_SPHERE) {
        return overlap_sphere_sphere(position, get_radius(i), other_position, get_radius(j));
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_PLANE) {
        return overlap_sphere_plane(position, get_radius(i), shape_other.plane.normal, shape_other.plane.offset);
    }

    if (collider->type == COLLIDER_PLANE && other_collider->type == COLLIDER_SPHERE) {
        return get_overlap(j, i);
    }

    if (collider->type == COLLIDER_CUBE && other_collider->type == COLLIDER_PLANE) {
        Penetration penetration = overlap_cuboid_plane(
            (Cuboid) {
                .center = position,
                .half_size = mult3(collider->radius, get_scale(i)),
                .rotation = get_rotation(i)
            },
            shape_other.plane
        );
        return penetration;
    }

    // LOG_ERROR("Invalid collision: %d vs %d", collider->type, other_collider->type);

    return (Penetration) {
        .valid = false
    };
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

        for (Entity j = 0; j < i; j++) {
            ColliderComponent* other_collider = get_component(j, COMPONENT_COLLIDER);
            if (!other_collider) continue;

            Penetration penetration = get_overlap(i, j);
            if (penetration.valid) {
                Collision collision = {
                    .entity = j,
                    .overlap = penetration.overlap,
                    .offset = diff3(penetration.contact_point, get_position(i)),
                    .offset_other = diff3(penetration.contact_point, get_position(j))
                };
                ArrayList_add(collider->collisions, &collision);
                Collision other_collision = {
                    .entity = i,
                    .overlap = mult3(-1.0f, penetration.overlap),
                    .offset = diff3(penetration.contact_point, get_position(j)),
                    .offset_other = diff3(penetration.contact_point, get_position(i))
                };
                ArrayList_add(other_collider->collisions, &other_collision);
            }
        }
    }
}
