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
    {3, 2, 1, 0}, // Back face
    {2, 6, 5, 1}, // Right face
    {6, 7, 4, 5}, // Front face
    {7, 3, 0, 4}, // Left face
    {7, 6, 2, 3}, // Top face
    {0, 1, 5, 4}  // Bottom face
};


ArrayList* face_groups;


bool faces_similar(CubeFace* a, CubeFace* b) {
    if (a->texture_index != b->texture_index) {
        return false;
    }

    if (a->material_index != b->material_index) {
        return false;
    }

    if (dot3(a->normal, b->normal) < 0.999f) {
        return false;
    }

    return true;
}


bool faces_adjacent(CubeFace* a, CubeFace* b) {
    if (dot3(a->normal, b->normal) < 0.999f) {
        return false;
    }

    // TODO: check if corner lies on edge
    int shared_corners = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (dist3(a->corners[i], b->corners[j]) < 0.001f) {
                shared_corners++;
            }
        }
    }

    return shared_corners >= 2;
}


float get_face_area(CubeFace* face) {
    Vector3 edge1 = sub3(face->corners[1], face->corners[0]);
    Vector3 edge2 = sub3(face->corners[3], face->corners[0]);
    return norm3(cross(edge1, edge2));
}


typedef struct FaceToRemove {
    bool first;
    bool second;
} FaceToRemove;


FaceToRemove faces_kissing(CubeFace* a, CubeFace* b) {
    int shared_corners = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (dist3(a->corners[i], b->corners[j]) < 0.01f) {
                shared_corners++;
            }
        }
    }

    if (shared_corners < 2) {
        return (FaceToRemove) { false, false };
    }

    float area_a = get_face_area(a);
    float area_b = get_face_area(b);

    // Areas are the same size
    if (fabsf(area_a - area_b) < 0.01f) {
        return (FaceToRemove) { true, true };
    }

    // Remove smaller face
    if (area_a < area_b) {
        return (FaceToRemove) { true, false };
    }

    return (FaceToRemove) { false, true };
}


void merge_adjacent_faces(ArrayList* face_group) {
    bool changed = true;
    while (changed) {
        changed = false;

        CubeFace* face = ArrayList_get(face_group, 0);
        for (int i = 1; i < face_group->size; i++) {
            CubeFace* other_face = ArrayList_get(face_group, i);
            if (faces_adjacent(face, other_face)) {
                LOG_INFO("Merging adjacent faces");
                // Merge logic here (not implemented)
                ArrayList_remove(face_group, i);
                changed = true;
                break;
            }
        }
    }
}


void add_to_face_group(CubeFace cube_face) {
    ArrayList* found_group = NULL;
    bool removed = false;
    for (int g = 0; g < face_groups->size; g++) {
        ArrayList* group = *(ArrayList**)ArrayList_get(face_groups, g);
        CubeFace* first_face = ArrayList_get(group, 0);
        if (!found_group && faces_similar(first_face, &cube_face)) {
            found_group = group;
        }
        if (dot3(first_face->normal, cube_face.normal) < -0.999f) {
            for (int i = 0; i < group->size; i++) {
                CubeFace* other_face = ArrayList_get(group, i);
                FaceToRemove result = faces_kissing(&cube_face, other_face);
                if (result.first) {
                    LOG_INFO("Removing first face");
                    removed = true;
                }
                if (result.second) {
                    LOG_INFO("Removing second face");
                    ArrayList_remove(group, i);
                    if (group->size == 0) {
                        ArrayList_remove(face_groups, g);
                        g--;
                    }
                }
            }
        }
    }

    if (removed) {
        return;
    }

    if (found_group) {
        ArrayList_add(found_group, &cube_face);
    } else {
        LOG_INFO("Creating new face group");
        found_group = ArrayList_create(sizeof(CubeFace));
        ArrayList_add(found_group, &cube_face);
        ArrayList_add(face_groups, &found_group);
    }
}


void create_face_groups() {
    LOG_INFO("Creating face groups");

    face_groups = ArrayList_create(sizeof(ArrayList*));
    models = ArrayList_create(sizeof(Model));

    int cube_index = binary_search_filename("cube", resources.mesh_names, resources.meshes_size);

    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        MeshComponent* mesh = get_component(entity, COMPONENT_MESH);
        if (!mesh) continue;
        if (mesh->mesh_index != cube_index) continue;

        Matrix4 transform = get_transform(entity);

        Vector3 scale = get_scale(entity);

        for (int f = 0; f < 6; f++) {
            Vector4 normal = cube_normals[f];
            normal = map4(transform, normal);

            CubeFace cube_face = {
                .normal = normalized3(vec4_xyz(normal)),
                .texture_index = mesh->texture_index,
                .material_index = mesh->material_index
            };

            // Determine tiling axes based on face normal
            Vector2 tiling = vec2(scale.x, scale.y);
            if (abs(cube_face.normal.x) > 0.5) {
                tiling = vec2(scale.z, scale.y);
            } else if (abs(cube_face.normal.y) > 0.5) {
                tiling = vec2(scale.x, scale.z);
            }

            for (int v = 0; v < 4; v++) {
                Vector4 corner = cube_corners[face_indices[f][v]];
                corner = map4(transform, corner);

                cube_face.corners[v] = vec4_xyz(corner);
                cube_face.uvs[v] = (Vector2){
                    .x = (v == 1 || v == 2) ? tiling.x : 0.0f,
                    .y = (v == 2 || v == 3) ? tiling.y : 0.0f
                };
            }

            cube_face.tangent = calculate_tangent(
                cube_face.corners[0],
                cube_face.corners[1],
                cube_face.corners[2],
                cube_face.uvs[0],
                cube_face.uvs[1],
                cube_face.uvs[2]
            );

            add_to_face_group(cube_face);
        }
    }

    for (int g = 0; g < face_groups->size; g++) {
        ArrayList* group = *(ArrayList**)ArrayList_get(face_groups, g);
        LOG_INFO("Face group %d has %d faces", g, group->size);
        CubeFace* first_face = ArrayList_get(group, 0);
        LOG_INFO("Texture index: %d, Material index: %d", first_face->texture_index, first_face->material_index);

        Mesh mesh_data = create_mesh_from_face_group(group);
        Mesh* mesh = malloc(sizeof(Mesh));
        *mesh = mesh_data;

        Model model = {
            .mesh = mesh,
            .instance_data = {
                .transform = identity4(),
                .texture_scale = ones2(),
                .texture_index = first_face->texture_index,
                .material_index = first_face->material_index,
                .emissive_index = -1,
                .visibility = VISIBILITY_ALL,
                .emissive = 0.0f
            }
        };
        ArrayList_add(models, &model);
    }

    // ArrayList_for_each(face_groups, ArrayList_destroy);
    // ArrayList_destroy(face_groups);
    face_groups = NULL;

    LOG_INFO("Finished creating face groups");
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
            if (mesh_component->mesh_index != cube_index) {
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
