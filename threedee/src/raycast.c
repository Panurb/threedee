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


Hit raycast(Ray ray) {
    Hit hit = {
        .entity = NULL_ENTITY,
        .distance = INFINITY,
        .point = zeros3(),
        .normal = zeros3()
    };

    for (int i = 0; i < scene->components->entities; i++) {
        ColliderComponent* collider = scene->components->collider[i];
        if (!collider) continue;

        switch (collider->type) {
            case COLLIDER_PLANE:
                break;
            case COLLIDER_SPHERE:
                Sphere sphere = { get_position(i), get_radius(i) };
                Intersection intersection = intersection_sphere_ray(sphere, ray);
                if (intersection.distance < hit.distance) {
                    hit.entity = i;
                    hit.distance = intersection.distance;
                    hit.point = intersection.point;
                    hit.normal = intersection.normal;
                    break;
                }
                break;
            CASE_COLLIDER_CUBOID:
                Cuboid cuboid = { get_position(i), get_half_extents(i), get_rotation(i) };

                break;
            default:
                LOG_ERROR("Unknown collider type: %d", collider->type);
        }
    }

    return hit;
}
