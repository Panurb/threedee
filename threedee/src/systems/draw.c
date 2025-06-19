#include "systems/draw.h"

#include <stdio.h>

#include "render.h"
#include "scene.h"
#include "util.h"


void draw_entities() {
    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        LightComponent* light = get_component(entity, COMPONENT_LIGHT);
        if (light) {
            Matrix4 view_matrix = transform_inverse(get_transform(entity));
            Matrix4 projection_matrix = light->projection_matrix;
            light->shadow_map.projection_view_matrix = matrix4_mult(projection_matrix, view_matrix);

            add_light(
                get_position(entity),
                light->diffuse_color,
                light->specular_color,
                light->shadow_map.projection_view_matrix
            );
        }

        MeshComponent* mesh_component = get_component(entity, COMPONENT_MESH);
        if (mesh_component) {
            render_mesh(
                get_transform(entity),
                mesh_component->mesh_index,
                mesh_component->texture_index,
                mesh_component->material_index
            );
        }

        if (light) {
            render_circle(
                get_position(entity),
                0.1f,
                32,
                COLOR_YELLOW
            );

            render_arrow(
                get_position(entity),
                sum3(get_position(entity), quaternion_forward(get_rotation(entity))),
                0.1f,
                COLOR_YELLOW
            );
        }

        ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
        RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
        if (entity != scene->player && collider && rb) {
            Vector3 start = get_position(entity);
            for (int i = 0; i < collider->collisions->size; i++) {
                Collision collision = *(Collision*)ArrayList_get(collider->collisions, i);
                Vector3 end = sum3(start, collision.overlap);
                render_arrow(start, end, 0.01f, COLOR_RED);

                end = sum3(start, collision.offset);
                render_arrow(start, end, 0.01f, COLOR_BLUE);
            }
        }
    }
}
