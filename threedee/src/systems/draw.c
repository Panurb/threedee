#include "systems/draw.h"

#include <systems/enemy.h>
#include <systems/navigation.h>

#include "app.h"
#include "render.h"
#include "scene.h"
#include "util.h"


void draw_axes(Entity entity) {
    if (!get_component(entity, COMPONENT_TRANSFORM)) {
        return; // No transform component, skip drawing axes
    }
    Vector3 pos = get_position(entity);

    if (dist3(pos, get_position(scene->camera)) < 0.1f) {
        return;
    }

    float thickness = 0.01f;
    float length = 0.2f;

    Vector3 x = vec3(length, 0.0f, 0.0f);
    Vector3 y = vec3(0.0f, length, 0.0f);
    Vector3 z = vec3(0.0f, 0.0f, length);

    Matrix3 rot = quaternion_to_rotation_matrix(get_rotation(entity));

    x = map3(rot, x);
    y = map3(rot, y);
    z = map3(rot, z);

    render_arrow(
        pos,
        add3(pos, x),
        thickness,
        COLOR_RED
    );

    render_arrow(
        pos,
        add3(pos, y),
        thickness,
        COLOR_GREEN
    );

    render_arrow(
        pos,
        add3(pos, z),
        thickness,
        COLOR_BLUE
    );
}


void draw_entities() {
    draw_circle_2d(
        zeros2(),
        0.007f,
        get_color(1.0f, 1.0f, 1.0f, 0.5f)
    );

    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        LightComponent* light = get_component(entity, COMPONENT_LIGHT);
        if (light && !light->disabled) {
            Matrix4 view_matrix = inverse_transform(get_transform(entity));
            Matrix4 projection_matrix = light->projection_matrix;
            light->shadow_map.projection_view_matrix = matrix4_mul(projection_matrix, view_matrix);

            add_light(entity);
        }

        MeshComponent* mesh_component = get_component(entity, COMPONENT_MESH);
        if (mesh_component) {
            Material material = resources.materials[mesh_component->material_index];
            if (light) {
                material.emissive = light->intensity;
            }
            draw_mesh(
                get_transform(entity),
                mesh_component->mesh_index,
                mesh_component->texture_index,
                material,
                mesh_component->emissive_index,
                mesh_component->visibility,
                mesh_component->texture_scale
            );
        }

        if (app.debug_level == 0) {
            continue;
        }

        draw_axes(entity);

        // debug_draw_enemies();

        ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
        RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
        if (entity != scene->player && collider && rb) {
            Vector3 start = get_position(entity);
            for (int i = 0; i < collider->collisions->size; i++) {
                Collision collision = *(Collision*)ArrayList_get(collider->collisions, i);
                Vector3 end = add3(start, collision.overlap);
                render_arrow(start, end, 0.01f, COLOR_RED);

                end = add3(start, collision.offset);
                render_arrow(start, end, 0.01f, COLOR_BLUE);
            }

            draw_collider(entity);
        }

        if (app.debug_level < 2) {
            continue;
        }

        draw_waypoints();

        if (app.debug_level < 3) {
            continue;
        }

        if (light) {
            render_circle(
                get_position(entity),
                0.1f,
                32,
                COLOR_YELLOW
            );

            Vector3 forward = quaternion_forward(get_rotation(entity));
            Vector3 up = vec3(0.0f, 1.0f, 0.0f);
            Vector3 right = cross(forward, up);
            up = cross(right, forward);

            Vector3 far_center = add3(get_position(entity), mul3(light->range, forward));
            float half_size = light->range * tanf(to_radians(light->fov) * 0.5f);
            Vector3 far_top_right = add3(far_center, mul3(half_size, add3(right, up)));
            Vector3 far_top_left = add3(far_center, mul3(half_size, sub3(right, up)));
            Vector3 far_bottom_right = add3(far_center, mul3(half_size, sub3(up, right)));
            Vector3 far_bottom_left = sub3(far_center, mul3(half_size, add3(right, up)));

            render_arrow(
                get_position(entity),
                far_top_left,
                0.1f,
                COLOR_YELLOW
            );
            render_arrow(
                get_position(entity),
                far_top_right,
                0.1f,
                COLOR_YELLOW
            );
            render_arrow(
                get_position(entity),
                far_bottom_left,
                0.1f,
                COLOR_YELLOW
            );
            render_arrow(
                get_position(entity),
                far_bottom_right,
                0.1f,
                COLOR_YELLOW
            );
        }
    }
}
