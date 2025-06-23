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


Intersection intersection_capsule_ray(Capsule capsule, Ray ray) {
    Intersection intersection = {
        .distance = INFINITY,
        .normal = zeros3()
    };

    // Transform ray to capsule's local space
    Matrix3 rot = quaternion_to_rotation_matrix(capsule.rotation);
    Matrix3 inv_rot = matrix3_inverse(rot);
    Vector3 local_origin = matrix3_map(inv_rot, diff3(ray.origin, capsule.center));
    Vector3 local_dir = matrix3_map(inv_rot, ray.direction);

    Vector3 p0 = mult3(-capsule.height * 0.5f, matrix3_column(rot, 1));
    Vector3 p1 = mult3(capsule.height * 0.5f, matrix3_column(rot, 1));

    // Check for intersection with the cylinder part
    Vector3 cylinder_axis = diff3(p1, p0);
    float cylinder_radius = capsule.radius;
    float a = dot3(local_dir, local_dir) - dot3(local_dir, cylinder_axis) * dot3(local_dir, cylinder_axis);
    Vector3 oc = diff3(local_origin, p0);
    float b = dot3(oc, local_dir) - dot3(oc, cylinder_axis) * dot3(local_dir, cylinder_axis);
    float c = dot3(oc, oc) - dot3(oc, cylinder_axis) * dot3(oc, cylinder_axis) - cylinder_radius * cylinder_radius;
    float discriminant = b * b - a * c;
    if (discriminant >= 0) {
        float t = (-b - sqrtf(discriminant)) / a;
        if (t >= 0) {
            intersection.distance = t;
            intersection.point = sum3(ray.origin, mult3(t, ray.direction));
            Vector3 hit_point = sum3(local_origin, mult3(t, local_dir));
            Vector3 closest_point = diff3(hit_point, mult3(dot3(hit_point, cylinder_axis), cylinder_axis));
            intersection.normal = normalized3(diff3(closest_point, p0));
            intersection.normal = matrix3_map(rot, intersection.normal);
            return intersection;
        }
    }
    // Check for intersection with the sphere caps
    Vector3 sphere_center0 = sum3(capsule.center, p0);
    Vector3 sphere_center1 = sum3(capsule.center, p1);
    Sphere sphere0 = { .center = sphere_center0, .radius = capsule.radius };
    Sphere sphere1 = { .center = sphere_center1, .radius = capsule.radius };
    Intersection intersection0 = intersection_sphere_ray(sphere0, ray);
    Intersection intersection1 = intersection_sphere_ray(sphere1, ray);
    if (intersection0.distance < intersection.distance) {
        intersection = intersection0;
        intersection.normal = normalized3(diff3(intersection.point, sphere_center0));
        intersection.normal = matrix3_map(rot, intersection.normal);
    }
    if (intersection1.distance < intersection.distance) {
        intersection = intersection1;
        intersection.normal = normalized3(diff3(intersection.point, sphere_center1));
        intersection.normal = matrix3_map(rot, intersection.normal);
    }
    return intersection;
}


Hit raycast(Ray ray, ColliderGroup group) {
    Hit hit = {
        .entity = NULL_ENTITY,
        .distance = INFINITY,
        .point = zeros3(),
        .normal = zeros3()
    };

    for (Entity i = 0; i < scene->components->entities; i++) {
        ColliderComponent* collider = get_component(i, COMPONENT_COLLIDER);
        if (!collider) continue;

        if (!(collider->group & group)) {
            continue;
        }

        Intersection intersection = {
            .distance = INFINITY
        };

        Shape shape = get_shape(i);

        switch (collider->type) {
            case COLLIDER_PLANE:
                break;
            case COLLIDER_SPHERE:
                intersection = intersection_sphere_ray(shape.sphere, ray);
                break;
            case COLLIDER_CUBOID:
                intersection = intersection_cuboid_ray(shape.cuboid, ray);
                break;
            case COLLIDER_CAPSULE:
                intersection = intersection_capsule_ray(shape.capsule, ray);
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
