#include "raycast.h"

#include <linalg.h>
#include <math.h>
#include <scene.h>
#include <stdio.h>
#include <systems/collision.h>


float intersection_sphere_ray(Sphere sphere, Ray ray) {
    Vector3 oc = diff3(ray.origin, sphere.center);
    float a = dot3(ray.direction, ray.direction);
    float b = 2.0f * dot3(oc, ray.direction);
    float c = dot3(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return INFINITY;
    }

    float t = (-b - sqrtf(discriminant)) / (2.0f * a);
    return t;
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
        if (!collider || collider->type != COLLIDER_SPHERE) {
            continue;
        }

        Sphere sphere = { get_position(i), get_radius(i) };
        float distance = intersection_sphere_ray(sphere, ray);

        if (distance < hit.distance) {
            Vector3 intersection = sum3(ray.origin, mult3(distance, ray.direction));
            Vector3 normal = normalized3(diff3(intersection, sphere.center));
            hit.entity = i;
            hit.distance = distance;
            hit.point = intersection;
            hit.normal = normal;
            break;
        }
    }

    return hit;
}
