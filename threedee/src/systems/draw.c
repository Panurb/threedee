#include <stdio.h>

#include "app.h"
#include "render.h"
#include "render_mesh.h"
#include "scene.h"
#include "util.h"
#include "systems/draw.h"
#include "systems/navigation.h"


static Vector4 cube_corners[8] = {
    {-0.5f, -0.5f, -0.5f, 1.0f},
    { 0.5f, -0.5f, -0.5f, 1.0f},
    { 0.5f,  0.5f, -0.5f, 1.0f},
    {-0.5f,  0.5f, -0.5f, 1.0f},
    {-0.5f, -0.5f,  0.5f, 1.0f},
    { 0.5f, -0.5f,  0.5f, 1.0f},
    { 0.5f,  0.5f,  0.5f, 1.0f},
    {-0.5f,  0.5f,  0.5f, 1.0f}
};

static Vector4 cube_normals[6] = {
    { 0.0f,  0.0f, -1.0f, 0.0f}, // Back
    { 1.0f,  0.0f,  0.0f, 0.0f}, // Right
    { 0.0f,  0.0f,  1.0f, 0.0f}, // Front
    {-1.0f,  0.0f,  0.0f, 0.0f}, // Left
    { 0.0f,  1.0f,  0.0f, 0.0f}, // Top
    { 0.0f, -1.0f,  0.0f, 0.0f}  // Bottom
};

static int face_indices[6][4] = {
    {0, 1, 2, 3}, // Back face
    {1, 5, 6, 2}, // Right face
    {5, 4, 7, 6}, // Front face
    {4, 0, 3, 7}, // Left face
    {3, 2, 6, 7}, // Top face
    {4, 5, 1, 0}  // Bottom face
};


ArrayList* face_groups;
ArrayList* meshes;


bool faces_adjacent(CubeFace* a, CubeFace* b) {
    MeshComponent* mesh_a = get_component(a->entity, COMPONENT_MESH);
    MeshComponent* mesh_b = get_component(b->entity, COMPONENT_MESH);

    if (mesh_a->texture_index != mesh_b->texture_index) {
        return false;
    }

    if (mesh_a->material_index != mesh_b->material_index) {
        return false;
    }

    if (dot3(a->normal, b->normal) < 0.999f) {
        return false;
    }

    return true;
}


void merge_adjacent_faces() {
    LOG_INFO("Merging adjacent cube faces");

    face_groups = ArrayList_create(sizeof(ArrayList));
    meshes = ArrayList_create(sizeof(Mesh));

    int cube_index = binary_search_filename("cube", resources.mesh_names, resources.meshes_size);

    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        MeshComponent* mesh = get_component(entity, COMPONENT_MESH);
        if (!mesh) continue;
        if (mesh->mesh_index != cube_index) continue;

        Matrix4 transform = get_transform(entity);

        for (int f = 0; f < 6; f++) {
            Vector4 normal = cube_normals[f];
            normal = map4(transform, normal);

            CubeFace cube_face = {
                .normal = normalized3(vec4_xyz(normal)),
                .tangent = vec3(1.0f, 0.0f, 0.0f), // Placeholder tangent
                .entity = entity
            };

            for (int v = 0; v < 4; v++) {
                Vector4 corner = cube_corners[face_indices[f][v]];
                corner = map4(transform, corner);

                cube_face.corners[v] = vec4_xyz(corner);
                cube_face.uvs[v] = (Vector2) {
                    (v == 0 || v == 3) ? 0.0f : 1.0f,
                    (v == 0 || v == 1) ? 1.0f : 0.0f
                };
            }

            ArrayList* found_group = NULL;
            for (int g = 0; g < face_groups->size; g++) {
                ArrayList* group = ArrayList_get(face_groups, g);
                CubeFace* first_face = ArrayList_get(group, 0);
                if (faces_adjacent(first_face, &cube_face)) {
                    found_group = group;
                    ArrayList_add(found_group, &cube_face);
                    break;
                }
            }

            if (!found_group) {
                LOG_INFO("Creating new face group for entity %d", entity);
                found_group = ArrayList_create(sizeof(CubeFace));
                ArrayList_add(found_group, &cube_face);
                ArrayList_add(face_groups, found_group);
            }
        }
    }

    for (int g = 0; g < face_groups->size; g++) {
        ArrayList* group = ArrayList_get(face_groups, g);
        LOG_INFO("Face group %d has %d faces", g, group->size);
        Vector3 normal = ((CubeFace*)ArrayList_get(group, 0))->normal;
        LOG_INFO("Normal: (%f, %f, %f)", normal.x, normal.y, normal.z);

        Mesh mesh = create_mesh_from_face_group(group);
        ArrayList_add(meshes, &mesh);
    }

    LOG_INFO("Finished merging adjacent cube faces");
}


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


void draw_springs(Entity entity) {
    RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
    if (!rb) return;

    // TODO: optimize
    int mesh_index = binary_search_filename("rope", resources.mesh_names, resources.meshes_size);
    int texture_index = binary_search_filename("black", resources.texture_names, resources.textures_size);
    int material_index = binary_search_filename("metal", resources.material_names, resources.materials_size);

    for (int i = 0; i < rb->springs->size; i++) {
        Spring spring = *(Spring*)ArrayList_get(rb->springs, i);
        if (spring.thickness == 0.0f) continue;

        Vector3 start = local_to_world(entity, spring.local_anchor);
        Vector3 end = spring.other_local_anchor;
        if (spring.entity != NULL_ENTITY) {
            end = local_to_world(spring.entity, spring.other_local_anchor);
        }

        Vector3 dir = sub3(end, start);
        float length = norm3(dir);
        Vector3 pos = mul3(0.5f, add3(start, end));
        Quaternion rotation = quaternion_from_forward(div3(length, dir), vec3_up());
        Vector3 scale = vec3(spring.thickness, spring.thickness, 0.5f * length);
        Matrix4 transform = transform_matrix(pos, rotation, scale);

        draw_mesh(
            transform,
            mesh_index,
            texture_index,
            material_index,
            -1,
            0.0f,
            VISIBILITY_ALL,
            ones2()
        );
    }
}


float get_emissive(Entity entity) {
    LightComponent* light = get_component(entity, COMPONENT_LIGHT);
    if (light && !light->disabled) {
        return light->intensity;
    }

    float emissive = 0.0f;
    TransformComponent* transform = get_component(entity, COMPONENT_TRANSFORM);
    ListNode* node;
    FOREACH(node, transform->children) {
        Entity child = node->value;
        emissive = fmax(emissive, get_emissive(child));
    }
    return emissive;
}


void draw_entities() {
    LOG_DEBUG("Drawing entities");

    draw_circle_2d(
        zeros2(),
        0.007f,
        get_color(1.0f, 1.0f, 1.0f, 0.5f)
    );

    int cube_index = binary_search_filename("cube", resources.mesh_names, resources.meshes_size);

    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        LightComponent* light = get_component(entity, COMPONENT_LIGHT);
        if (light && !light->disabled) {
            Matrix4 view_matrix = inverse_transform(get_transform(entity));
            Matrix4 projection_matrix = light->projection_matrix;
            light->shadow_map.projection_view_matrix = matrix4_mul(projection_matrix, view_matrix);

            add_light(entity);
        }

        MeshComponent* mesh_component = get_component(entity, COMPONENT_MESH);
        if (mesh_component && mesh_component->visible) {
            float emissive = get_emissive(entity);
            if (mesh_component->mesh_index == cube_index) {
                draw_cube(
                    get_transform(entity),
                    CubeIndices_fill(mesh_component->texture_index),
                    CubeIndices_fill(mesh_component->material_index)
                );
            } else {
                draw_mesh(
                    get_transform(entity),
                    mesh_component->mesh_index,
                    mesh_component->texture_index,
                    mesh_component->material_index,
                    mesh_component->emissive_index,
                    emissive,
                    mesh_component->visibility,
                    mesh_component->texture_scale
                );
            }
        }

        SpriteComponent* sprite = get_component(entity, COMPONENT_SPRITE);
        if (sprite) {
            draw_sprite(
                get_position(entity),
                1.0f,
                1.0f,
                sprite->texture_index
            );
        }

        draw_springs(entity);

        if (app.debug_level == 0) {
            continue;
        }

        // draw_axes(entity);

        // debug_draw_enemies();

        RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
        ColliderComponent* collider = get_component(entity, COMPONENT_COLLIDER);
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

        if (rb) {
            for (int i = 0; i < rb->springs->size; i++) {
                Spring spring = *(Spring*)ArrayList_get(rb->springs, i);
                if (spring.thickness != 0.0f) continue;

                Vector3 start = local_to_world(entity, spring.local_anchor);
                Vector3 end = spring.other_local_anchor;
                if (spring.entity != NULL_ENTITY) {
                    end = local_to_world(spring.entity, spring.other_local_anchor);
                }
                draw_line(start, end, 0.02f, COLOR_ORANGE);
            }
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
