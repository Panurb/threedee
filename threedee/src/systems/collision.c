#include "systems/collision.h"

#include <render.h>
#include <stdio.h>

#include "scene.h"
#include "util.h"


static const unsigned int COLLISION_MASKS[] = {
    [GROUP_WALLS] = 0,
    [GROUP_PLAYERS] = GROUP_WALLS,
    [GROUP_PROPS] = GROUP_WALLS | GROUP_PLAYERS | GROUP_PROPS
};


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
        { -cuboid.half_extents.x, -cuboid.half_extents.y, -cuboid.half_extents.z },
        {  cuboid.half_extents.x, -cuboid.half_extents.y, -cuboid.half_extents.z },
        {  cuboid.half_extents.x,  cuboid.half_extents.y, -cuboid.half_extents.z },
        { -cuboid.half_extents.x,  cuboid.half_extents.y, -cuboid.half_extents.z },
        { -cuboid.half_extents.x, -cuboid.half_extents.y,  cuboid.half_extents.z },
        {  cuboid.half_extents.x, -cuboid.half_extents.y,  cuboid.half_extents.z },
        {  cuboid.half_extents.x,  cuboid.half_extents.y,  cuboid.half_extents.z },
        { -cuboid.half_extents.x,  cuboid.half_extents.y,  cuboid.half_extents.z }
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
    Vector3 half_extents = cuboid.half_extents;
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


Vector3 intersect(Vector3 p1, Vector3 p2, Plane plane) {
    float d1 = dot3(p1, plane.normal) - plane.offset;
    float d2 = dot3(p2, plane.normal) - plane.offset;
    float t = d1 / (d1 - d2);
    return sum3(p1, mult3(t, diff3(p2, p1)));
}


void clip_polygon(PolygonShape* polygon, Plane plane) {
    // Sutherland-Hodgman algorithm for clipping a polygon against a plane
    if (polygon->size == 0) {
        return;
    }

    #define inside(p) (dot3(p, plane.normal) <= plane.offset)

    // The clipped polygon will have at most 1 extra point
    PolygonShape clipped = {
        .points = malloc(sizeof(Vector3) * (polygon->size + 4)),
        .size = 0
    };

    Vector3 prev = polygon->points[polygon->size - 1];
    for (int i = 0; i < polygon->size; i++) {
        Vector3 curr = polygon->points[i];
        if (inside(curr)) {
            if (!inside(prev)) {
                clipped.points[clipped.size++] = intersect(prev, curr, plane);
            }
            clipped.points[clipped.size++] = curr;
        } else if (inside(prev)) {
            clipped.points[clipped.size++] = intersect(prev, curr, plane);
        }
        prev = curr;
    }
    free(polygon->points);
    polygon->points = clipped.points;
    polygon->size = clipped.size;
    #undef inside
}


Vector3 contact_point_cuboid_cuboid(Cuboid cuboid1, Cuboid cuboid2, Vector3 overlap_axis) {
    Matrix3 rot1 = quaternion_to_rotation_matrix(cuboid1.rotation);
    Matrix3 rot2 = quaternion_to_rotation_matrix(cuboid2.rotation);

    // Choose reference cuboid and axis which is most perpendicular to the overlap axis
    float dot_refs[6];
    for (int i = 0; i < 3; i++) {
        dot_refs[i] = dot3(overlap_axis, matrix3_column(rot1, i));
        dot_refs[i + 3] = dot3(overlap_axis, matrix3_column(rot2, i));
    }
    int ref_axis = abs_argmax(dot_refs, 6);
    float ref_sign = sign(dot_refs[ref_axis]);

    Cuboid ref_cuboid = cuboid1;
    Cuboid inc_cuboid = cuboid2;
    Matrix3 ref_rot = rot1;
    Matrix3 inc_rot = rot2;
    if (ref_axis >= 3) {
        ref_cuboid = cuboid2;
        inc_cuboid = cuboid1;
        ref_rot = rot2;
        inc_rot = rot1;
        ref_axis -= 3;
        // Pick face closest to collision
        ref_sign *= -1.0f;
    }

    Vector3 ref_face_normal = mult3(-ref_sign, matrix3_column(ref_rot, ref_axis));
    Vector3 ref_face_center = sum3(
        ref_cuboid.center,
        mult3(vec3_get(ref_cuboid.half_extents, ref_axis), ref_face_normal)
    );

    int ref_face_axes[2] = {
        (ref_axis + 1) % 3,
        (ref_axis + 2) % 3
    };

    Vector3 ref_face_points[4];
    int n = 0;
    for (int dx = -1; dx <= 1; dx += 2) {
        for (int dy = -1; dy <= 1; dy += 2) {
            Vector3 offset = sum3(
                mult3(dx * vec3_get(ref_cuboid.half_extents, ref_face_axes[0]), matrix3_column(ref_rot, ref_face_axes[0])),
                mult3(dy * vec3_get(ref_cuboid.half_extents, ref_face_axes[1]), matrix3_column(ref_rot, ref_face_axes[1]))
            );
            ref_face_points[n] = sum3(ref_face_center, offset);
            n++;
        }
    }

    float dot_incs[3];
    for (int i = 0; i < 3; i++) {
        dot_incs[i] = dot3(ref_face_normal, matrix3_column(inc_rot, i));
    }
    int inc_axis = abs_argmax(dot_incs, 3);
    float inc_sign = sign(dot_incs[inc_axis]);

    Vector3 inc_face_center = sum3(
        inc_cuboid.center,
        mult3(-inc_sign * vec3_get(inc_cuboid.half_extents, inc_axis), matrix3_column(inc_rot, inc_axis))
    );

    int inc_face_axes[2] = {
        (inc_axis + 1) % 3,
        (inc_axis + 2) % 3
    };

    Vector3 inc_face_points[4];
    n = 0;
    for (int dx = -1; dx <= 1; dx += 2) {
        for (int dy = -1; dy <= 1; dy += 2) {
            Vector3 offset = sum3(
                mult3(dx * vec3_get(inc_cuboid.half_extents, inc_face_axes[0]), matrix3_column(inc_rot, inc_face_axes[0])),
                mult3(dy * vec3_get(inc_cuboid.half_extents, inc_face_axes[1]), matrix3_column(inc_rot, inc_face_axes[1]))
            );
            inc_face_points[n++] = sum3(inc_face_center, offset);
        }
    }

    Vector3 ref_u = matrix3_column(ref_rot, ref_face_axes[0]);
    Vector3 ref_v = matrix3_column(ref_rot, ref_face_axes[1]);
    float hu = vec3_get(ref_cuboid.half_extents, ref_face_axes[0]);
    float hv = vec3_get(ref_cuboid.half_extents, ref_face_axes[1]);

    PolygonShape clipped = {
        .points = malloc(sizeof(Vector3) * 4),
        .size = 4
    };
    memcpy(clipped.points, inc_face_points, sizeof(Vector3) * 4);

    Plane planes[4] = {
        { ref_u, dot3(ref_u, sum3(ref_face_center, mult3(hu, ref_u))) },
        { neg3(ref_u), dot3(neg3(ref_u), diff3(ref_face_center, mult3(hu, ref_u))) },
        { ref_v, dot3(ref_v, sum3(ref_face_center, mult3(hv, ref_v))) },
        { neg3(ref_v), dot3(neg3(ref_v), diff3(ref_face_center, mult3(hv, ref_v))) }
    };

    for (int i = 0; i < 4; i++) {
        clip_polygon(&clipped, planes[i]);
        if (clipped.size == 0) {
            break;
        }
    }

    Vector3 contact_normal = normalized3(ref_face_normal);
    Vector3 avg_contact_point = zeros3();
    int contact_count = 0;
    for (int i = 0; i < clipped.size; i++) {
        Vector3 p = clipped.points[i];
        float depth = dot3(diff3(p, ref_face_center), contact_normal);
        if (depth > 1e-4f) {
            continue;
        }

        Vector3 contact_point = diff3(p, mult3(depth, contact_normal));
        avg_contact_point = sum3(avg_contact_point, contact_point);
        contact_count++;
    }
    if (contact_count > 0) {
        avg_contact_point = div3((float)contact_count, avg_contact_point);
    }

    free(clipped.points);
    return avg_contact_point;
}


Penetration penetration_cuboid_cuboid(Cuboid cuboid1, Cuboid cuboid2) {
    const float eps = 1e-4f;

    Matrix3 rot1 = quaternion_to_rotation_matrix(cuboid1.rotation);
    Matrix3 inv_rot1 = transpose3(rot1);
    Matrix3 rot2 = quaternion_to_rotation_matrix(cuboid2.rotation);
    Vector3 t_world = diff3(cuboid2.center, cuboid1.center);

    // Cuboid 2 rotation in cuboid 1 local frame
    Matrix3 rot = matrix3_mult(inv_rot1, rot2);

    // Translation vector in cuboid 1 local frame
    Vector3 t = matrix3_map(inv_rot1, t_world);

    // Compute common subexpression
    Matrix3 abs_rot = matrix3_add_scalar(matrix3_abs(rot), eps);

    Penetration penetration = {
        .valid = false
    };

    float min_overlap = INFINITY;
    Vector3 overlap_axis = zeros3();

    // Test axes L = A0, A1, A2 (cuboid 1 local axes)
    for (int i = 0; i < 3; i++) {
        float ra = vec3_get(cuboid1.half_extents, i);
        float rb = dot3(cuboid2.half_extents, matrix3_row(abs_rot, i));

        float overlap = ra + rb - fabsf(vec3_get(t, i));
        if (overlap < 0.0f) {
            return penetration;
        }

        if (overlap < min_overlap) {
            min_overlap = overlap;
            overlap_axis = matrix3_column(rot1, i);
        }
    }

    // Test axes L = B0, B1, B2 (cuboid 2 local axes)
    for (int i = 0; i < 3; i++) {
        float ra = dot3(cuboid1.half_extents, matrix3_row(abs_rot, i));
        float rb = vec3_get(cuboid2.half_extents, i);

        float overlap = ra + rb - fabsf(dot3(t, matrix3_column(rot, i)));
        if (overlap < 0.0f) {
            return penetration;
        }

        if (overlap < min_overlap) {
            min_overlap = overlap;
            overlap_axis = matrix3_column(rot2, i);
        }
    }

    // Test axes L = A0 x B0, ..., A2 x B2
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Vector3 axis = cross(matrix3_column(rot1, i), matrix3_column(rot2, j));
            float axis_norm = norm3(axis);
            if (axis_norm < eps) continue;
            axis = mult3(1.0f / axis_norm, axis);

            float ra = 0.0f;
            float rb = 0.0f;

            for (int k = 0; k < 3; k++) {
                ra += fabsf(vec3_get(cuboid1.half_extents, k) * dot3(axis, matrix3_row(rot1, k)));
                rb += fabsf(vec3_get(cuboid2.half_extents, k) * dot3(axis, matrix3_row(rot2, k)));
            }

            float dist = fabsf(dot3(t, axis));
            float overlap = ra + rb - dist;
            if (overlap < 0.0f) {
                return penetration;
            }

            if (overlap < min_overlap) {
                min_overlap = overlap;
                overlap_axis = axis;
            }
        }
    }

    if (dot3(t_world, overlap_axis) > 0.0f) {
        overlap_axis = mult3(-1.0f, overlap_axis);
    }

    penetration.valid = true;
    penetration.overlap = mult3(min_overlap, overlap_axis);
    penetration.contact_point = contact_point_cuboid_cuboid(cuboid1, cuboid2, overlap_axis);

    return penetration;
}


Penetration penetration_capsule_plane(Capsule capsule, Plane plane) {
    Penetration penetration = {
        .valid = false
    };

    Matrix3 rot = quaternion_to_rotation_matrix(capsule.rotation);
    Vector3 up = matrix3_column(rot, 1);
    Vector3 p0 = sum3(capsule.center, mult3(-capsule.height * 0.5f, up));
    Vector3 p1 = sum3(capsule.center, mult3(capsule.height * 0.5f, up));
    float dist0 = dot3(p0, plane.normal) - plane.offset;
    float dist1 = dot3(p1, plane.normal) - plane.offset;

    float denom = dist0 - dist1;
    float t;
    if (fabsf(denom) > 1e-6f) {
        t = dist0 / denom;
        t = clamp(t, 0.0f, 1.0f);
    } else {
        t = (fabsf(dist0) < fabsf(dist1)) ? 0.0f : 1.0f;
    }

    Vector3 closest_point = lerp3(p0, p1, t);
    float closest_dist = dot3(closest_point, plane.normal) - plane.offset;

    float penetration_depth = capsule.radius - closest_dist;
    if (penetration_depth > 0.0f) {
        penetration.valid = true;
        penetration.overlap = mult3(penetration_depth, plane.normal);
        penetration.contact_point = diff3(closest_point, mult3(capsule.radius, plane.normal));
    }

    return penetration;
}


Penetration penetration_sphere_aabb(Sphere sphere, AABB aabb) {
    Penetration penetration = {
        .valid = false
    };

    Vector3 half_extents = aabb.half_extents;
    Vector3 closest_point = clamp3(sphere.center, diff3(aabb.center, half_extents), sum3(aabb.center, half_extents));

    Sphere point = {
        .center = closest_point,
        .radius = 0.0f
    };
    Penetration sphere_penetration = penetration_sphere_sphere(sphere, point);
    if (sphere_penetration.valid) {
        sphere_penetration.contact_point = closest_point;
        return sphere_penetration;
    }

    return penetration;
}


Vector3 contact_point_obb_aabb(Cuboid obb, AABB aabb, Vector3 normal) {
    Matrix3 rot_obb = quaternion_to_rotation_matrix(obb.rotation);
    Vector3 contact_point = zeros3();
    float max_penetration = -INFINITY;

    // Compute all 8 corners of the OBB
    for (int dx = -1; dx <= 1; dx += 2) {
        for (int dy = -1; dy <= 1; dy += 2) {
            for (int dz = -1; dz <= 1; dz += 2) {
                Vector3 offset = sum3(
                    mult3(dx * obb.half_extents.x, matrix3_column(rot_obb, 0)),
                    sum3(
                        mult3(dy * obb.half_extents.y, matrix3_column(rot_obb, 1)),
                        mult3(dz * obb.half_extents.z, matrix3_column(rot_obb, 2))
                    )
                );
                Vector3 corner = sum3(obb.center, offset);

                // Vector from AABB to OBB corner
                Vector3 diff = diff3(corner, aabb.center);
                float depth = -dot3(diff, normal);  // Negative = into AABB

                if (depth > max_penetration) {
                    max_penetration = depth;
                    contact_point = corner;
                }
            }
        }
    }

    // Project deepest point onto AABB face (by removing normal component of penetration)
    contact_point = sum3(contact_point, mult3(max_penetration, normal));

    // Clamp to AABB bounds along the tangent directions (to get a valid surface point)
    Vector3 aabb_min = diff3(aabb.center, aabb.half_extents);
    Vector3 aabb_max = sum3(aabb.center, aabb.half_extents);

    for (int i = 0; i < 3; i++) {
        // If normal is mostly aligned with this axis, skip clamping that axis
        if (fabsf(vec3_get(normal, i)) > 0.99f) continue;

        float v = vec3_get(contact_point, i);
        v = fmaxf(vec3_get(aabb_min, i), fminf(v, vec3_get(aabb_max, i)));
        vec3_set(&contact_point, i, v);
    }

    return contact_point;
}



Penetration penetration_cuboid_aabb(Cuboid cuboid, AABB aabb) {
    Cuboid cuboid_aabb = {
        .center = aabb.center,
        .half_extents = aabb.half_extents,
        .rotation = quaternion_id(),
    };

    Penetration penetration = penetration_cuboid_cuboid(cuboid, cuboid_aabb);
    penetration.contact_point = contact_point_obb_aabb(cuboid, aabb, penetration.overlap);

    return penetration;
}


Penetration penetration_capsule_aabb(Capsule capsule, AABB aabb) {
    Penetration penetration = {
        .valid = false
    };

    Vector3 half_extents = aabb.half_extents;
    Vector3 closest_point = clamp3(capsule.center, diff3(aabb.center, half_extents), sum3(aabb.center, half_extents));

    Sphere point = {
        .center = closest_point,
        .radius = 0.0f
    };
    Penetration sphere_penetration = penetration_sphere_sphere((Sphere) { .center = capsule.center, .radius = capsule.radius }, point);
    if (sphere_penetration.valid) {
        sphere_penetration.contact_point = closest_point;
        return sphere_penetration;
    }

    return penetration;
}


Penetration get_penetration(Entity i, Entity j) {
    ColliderComponent* collider = scene->components->collider[i];
    ColliderComponent* other_collider = scene->components->collider[j];

    if (!collider || !other_collider) {
        return (Penetration) {
            .valid = false
        };
    }

    Shape shape = get_shape(i);
    Shape shape_other = get_shape(j);

    if (collider->type == COLLIDER_PLANE && other_collider->type != COLLIDER_PLANE) {
        Penetration penetration = get_penetration(j, i);
        penetration.overlap = neg3(penetration.overlap);
        return penetration;
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_SPHERE) {
        return penetration_sphere_sphere(shape.sphere, shape_other.sphere);
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_PLANE) {
        return penetration_sphere_plane(shape.sphere, shape_other.plane);
    }

    if (collider->type == COLLIDER_CUBOID && other_collider->type == COLLIDER_PLANE) {
        Penetration penetration = penetration_cuboid_plane(shape.cuboid, shape_other.plane);
        return penetration;
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_CUBOID) {
        return penetration_sphere_cuboid(shape.sphere, shape_other.cuboid);
    }

    if (collider->type == COLLIDER_CUBOID && other_collider->type == COLLIDER_SPHERE) {
        Penetration penetration = penetration_sphere_cuboid(shape_other.sphere, shape.cuboid);
        penetration.overlap = neg3(penetration.overlap);
        return penetration;
    }

    if (collider->type == COLLIDER_CUBOID && other_collider->type == COLLIDER_CUBOID) {
        return penetration_cuboid_cuboid(shape.cuboid, shape_other.cuboid);
    }

    if (collider->type == COLLIDER_CAPSULE && other_collider->type == COLLIDER_PLANE) {
        return penetration_capsule_plane(shape.capsule, shape_other.plane);
    }

    if (collider->type == COLLIDER_SPHERE && other_collider->type == COLLIDER_AABB) {
        return penetration_sphere_aabb(shape.sphere, shape_other.aabb);
    }

    if (collider->type == COLLIDER_AABB && other_collider->type == COLLIDER_SPHERE) {
        Penetration penetration = penetration_sphere_aabb(shape_other.sphere, shape.aabb);
        penetration.overlap = neg3(penetration.overlap);
        return penetration;
    }

    if (collider->type == COLLIDER_CUBOID && other_collider->type == COLLIDER_AABB) {
        return penetration_cuboid_aabb(shape.cuboid, shape_other.aabb);
    }

    if (collider->type == COLLIDER_AABB && other_collider->type == COLLIDER_CUBOID) {
        Penetration penetration = penetration_cuboid_aabb(shape_other.cuboid, shape.aabb);
        penetration.overlap = neg3(penetration.overlap);
        return penetration;
    }

    if (collider->type == COLLIDER_CAPSULE && other_collider->type == COLLIDER_AABB) {
        return penetration_capsule_aabb(shape.capsule, shape_other.aabb);
    }

    if (collider->type == COLLIDER_AABB && other_collider->type == COLLIDER_CAPSULE) {
        Penetration penetration = penetration_capsule_aabb(shape_other.capsule, shape.aabb);
        penetration.overlap = neg3(penetration.overlap);
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

            // TODO: Handle unsymmetric collisions
            bool collides = (COLLISION_MASKS[collider->group] & other_collider->group);
            bool other_collides = (COLLISION_MASKS[other_collider->group] & collider->group);

            if (!collides && !other_collides) {
                continue;
            }

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
