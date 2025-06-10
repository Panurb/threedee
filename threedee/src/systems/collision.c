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


Penetration penetration_sphere_sphere(Sphere sphere1, Sphere sphere2) {
    Penetration penetration = {
        .valid = false
    };

    Vector3 diff = diff3(sphere1.center, sphere2.center);
    float dist_sq = dot3(diff, diff);
    float radius_sum = sphere1.radius + sphere2.radius;
    if (dist_sq < radius_sum * radius_sum) {
        float dist = sqrtf(dist_sq);
        if (dist == 0.0f) {
            return penetration;
        }
        penetration.valid = true;
        penetration.overlap = mult3((radius_sum - dist) / dist, diff);
        penetration.contact_point = sum3(sphere1.center, mult3(sphere1.radius / radius_sum, diff));
    }

    return penetration;
}


Penetration penetration_sphere_plane(Sphere sphere, Plane plane) {
    Penetration penetration = {
        .valid = false
    };
    float dist = dot3(sphere.center, plane.normal) - plane.offset;
    if (dist < sphere.radius) {
        penetration.valid = true;
        penetration.overlap = mult3(sphere.radius - dist, plane.normal);
        penetration.contact_point = diff3(sphere.center, mult3(sphere.radius, plane.normal));
    }
    return penetration;
}


Penetration penetration_cuboid_plane(Cuboid cuboid, Plane plane) {
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


Penetration penetration_sphere_cuboid(Sphere sphere, Cuboid cuboid) {
    Vector3 half_extents = cuboid.half_size;
    Vector3 closest_point = clamp3(sphere.center, diff3(cuboid.center, half_extents), sum3(cuboid.center, half_extents));

    Sphere point = {
        .center = closest_point,
        .radius = 0.0f
    };
    Penetration penetration = penetration_sphere_sphere(sphere, point);
    if (penetration.valid) {
        penetration.contact_point = closest_point;
    }
    return penetration;
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
        case COLLIDER_SPHERE: {
            shape.sphere = (Sphere) {
                .center = position,
                .radius = radius
            };
            break;
        }
        case COLLIDER_CUBE: {
            shape.cuboid = (Cuboid) {
                .center = position,
                .half_size = mult3(radius, get_scale(entity)),
                .rotation = get_rotation(entity),
            };
            break;
        }
    }

    return shape;
}


Penetration get_penetration(Entity i, Entity j) {
    ColliderComponent* collider = scene->components->collider[i];
    ColliderComponent* other_collider = scene->components->collider[j];

    if (!collider || !other_collider) {
        return (Penetration) {
            .valid = false
        };
    }

    Vector3 position = get_position(i);
    Vector3 other_position = get_position(j);

    Shape shape = get_shape(i);
    Shape shape_other = get_shape(j);

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_SPHERE) {
        return penetration_sphere_sphere(shape.sphere, shape_other.sphere);
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_PLANE) {
        return penetration_sphere_plane(shape.sphere, shape_other.plane);
    }

    if (collider->type == COLLIDER_CUBE && other_collider->type == COLLIDER_PLANE) {
        Penetration penetration = penetration_cuboid_plane(shape.cuboid, shape_other.plane);
        return penetration;
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_CUBE) {
        return penetration_sphere_cuboid(shape.sphere, shape_other.cuboid);
    }

    if (collider->type == COLLIDER_CUBE && other_collider->type == COLLIDER_SPHERE) {
        Penetration penetration = penetration_sphere_cuboid(shape_other.sphere, shape.cuboid);
        penetration.overlap = mult3(-1.0f, penetration.overlap);
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

            Penetration penetration = get_penetration(i, j);
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
