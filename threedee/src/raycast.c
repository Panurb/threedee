#include "raycast.h"

#include <linalg.h>
#include <math.h>
#include <scene.h>
#include <stdio.h>
#include <systems/collision.h>


Intersection intersection_sphere_ray(Sphere sphere, Ray ray) {
    Intersection intersection = {
        .distance = INFINITY,
        .normal = zeros3()
    };

    Vector3 oc = diff3(ray.origin, sphere.center);
    float a = dot3(ray.direction, ray.direction);
    float b = 2.0f * dot3(oc, ray.direction);
    float c = dot3(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return intersection;
    }

    intersection.distance = (-b - sqrtf(discriminant)) / (2.0f * a);
    intersection.point = sum3(ray.origin, mult3(intersection.distance, ray.direction));
    intersection.normal = normalized3(diff3(intersection.point, sphere.center));

    return intersection;
}

Intersection intersection_cuboid_ray(Cuboid cuboid, Ray ray) {
    Intersection intersection = {
        .distance = INFINITY,
        .normal = zeros3()
    };

    // Transform ray to cuboid's local space
    Matrix3 rot = quaternion_to_rotation_matrix(cuboid.rotation);
    Matrix3 inv_rot = matrix3_inverse(rot);
    Vector3 local_origin = matrix3_map(inv_rot, diff3(ray.origin, cuboid.center));
    Vector3 local_dir = matrix3_map(inv_rot, ray.direction);

    Vector3 min = mult3(-1.0f, cuboid.half_extents);
    Vector3 max = cuboid.half_extents;

    float tmin = -INFINITY, tmax = INFINITY;
    int hit_axis = -1;

    for (int i = 0; i < 3; i++) {
        float o = vec3_get(local_origin, i);
        float d = vec3_get(local_dir, i);
        float min_i = vec3_get(min, i);
        float max_i = vec3_get(max, i);

        if (fabsf(d) < 1e-6f) {
            if (o < min_i || o > max_i) return intersection;
        } else {
            float t1 = (min_i - o) / d;
            float t2 = (max_i - o) / d;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) { tmin = t1; hit_axis = i; }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax || tmax < 0) return intersection;
        }
    }

    intersection.distance = tmin > 0 ? tmin : tmax;
    intersection.point = sum3(ray.origin, mult3(intersection.distance, ray.direction));

    // Compute normal in local space, then rotate back
    Vector3 local_hit = sum3(local_origin, mult3(intersection.distance, local_dir));
    Vector3 local_normal = zeros3();
    vec3_set(&local_normal, hit_axis, (vec3_get(local_hit, hit_axis) > 0) ? 1.0f : -1.0f);
    intersection.normal = matrix3_map(rot, local_normal);

    return intersection;
}


Hit raycast(Ray ray) {
    Hit hit = {
        .entity = NULL_ENTITY,
        .distance = INFINITY,
        .point = zeros3(),
        .normal = zeros3()
    };

    for (Entity i = 0; i < scene->components->entities; i++) {
        ColliderComponent* collider = get_component(i, COMPONENT_COLLIDER);
        if (!collider) continue;

        Intersection intersection = {
            .distance = INFINITY
        };
        switch (collider->type) {
            case COLLIDER_PLANE:
                break;
            case COLLIDER_SPHERE:
                Sphere sphere = { get_position(i), get_radius(i) };
                intersection = intersection_sphere_ray(sphere, ray);
                break;
            case COLLIDER_CUBE:
                Cuboid cuboid = { get_position(i), get_half_extents(i), get_rotation(i) };
                intersection = intersection_cuboid_ray(cuboid, ray);
                break;
            default:
                LOG_ERROR("Unknown collider type: %d", collider->type);
        }

        if (intersection.distance < hit.distance) {
            hit.entity = i;
            hit.distance = intersection.distance;
            hit.point = intersection.point;
            hit.normal = intersection.normal;
        }
    }

    return hit;
}
