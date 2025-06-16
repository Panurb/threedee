#include "systems/draw.h"

#include <stdio.h>

#include "render.h"
#include "scene.h"
#include "util.h"


void draw_entities() {
    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        LightComponent* light_component = get_component(entity, COMPONENT_LIGHT);
        if (light_component) {
            Matrix4 view_matrix = look_at_matrix(
                get_position(entity),
                zeros3(),
                (Vector3){ 0.0f, 1.0f, 0.0f }
            );
            Matrix4 projection_matrix = orthographic_projection_matrix(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 10.0f);
            light_component->shadow_map.projection_view_matrix = matrix4_mult(projection_matrix, view_matrix);

            add_light(
                get_position(entity),
                light_component->diffuse_color,
                light_component->specular_color,
                light_component->shadow_map.projection_view_matrix
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

        ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
        RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
        if (collider && rb) {
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
