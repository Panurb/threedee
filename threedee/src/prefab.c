#include "component.h"
#include "linalg.h"
#include "quaternion.h"


Entity create_lamp(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "lamp",
        .texture_filename = "lamp",
        .material_filename = "metal",
        .emissive_filename = "lamp"
    });
    RigidBodyComponent_add(i, (RigidBodyParameters) {
        .mass = 1.0f,
        .friction = 0.5f,
        .bounce = 0.5f
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_SPHERE,
        .group = GROUP_PROPS,
        .radius = 0.25f
    });
    add_spring(i, (Spring) {
        .entity = NULL_ENTITY,
        .local_anchor = vec3(0.0f, 0.2f, 0.0f),
        .other_local_anchor = add3(position, vec3(0.0f, 0.5f, 0.0f)),
        .rest_length = 0.0f,
        .stiffness = 50.0f,
        .damping = 1.0f,
        .thickness = 0.015f
    });

    Entity light = create_entity();
    TransformComponent_add(light, (TransformParameters) {
        .rotation = quaternion_from_forward(vec3_down(), vec3_forward()),
        .parent = i
    });
    LightComponent_add(light, (LightParameters) {
        .color = COLOR_WHITE,
        .visibility_mask = VISIBILITY_NORMAL,
        .range = 10.0f
    });

    Entity force = create_entity();
    TransformComponent_add(force, (TransformParameters) {
        .position = position,
    });
    ColliderComponent_add(force, (ColliderParameters) {
        .type = COLLIDER_SPHERE,
        .group = GROUP_NONE,
        .radius = 1.0f
    });
    ForceComponent_add(force, (ForceParameters) {
        .direction = vec3_forward(),
        .magnitude = 10.0f,
        .disabled = true,
        .duration = 0.1f
    });

    Entity trigger = create_entity();
    TransformComponent_add(trigger, (TransformParameters) {
        .position = position,
    });
    TriggerComponent_add(trigger, (TriggerParameters) {
        .type = TRIGGER_MANUAL,
        .on_enter = enable_force,
        .target_entity = force
    });

    return i;
}


Entity create_television(Vector3 position, float yaw) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
        .yaw = yaw
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "television",
        .texture_filename = "television",
        .material_filename = "plastic",
        .emissive_filename = "television"
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_CUBOID,
        .group = GROUP_WALLS,
        .width = 1.8f,
        .height = 2.0f,
        .depth = 0.2f
    });
    LightComponent_add(i, (LightParameters) {
        .color = COLOR_WHITE,
        .fov = 90.0f,
        .range = 5.0f,
        .intensity = 0.25f
    });

    return i;
}


Entity create_chair(Vector3 position, float yaw) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = add3(position, vec3(0.0f, 0.75f, 0.0f)),
        .yaw = yaw
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "chair",
        .texture_filename = "wood",
        .material_filename = "concrete"
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_CUBOID,
        .group = GROUP_PROPS,
        .width = 0.75f,
        .height = 1.5f,
        .depth = 0.9f
    });
    RigidBodyComponent_add(i, (RigidBodyParameters) {
        .mass = 5.0f,
        .friction = 0.5f,
        .bounce = 0.2f
    });
    SoundComponent_add(i, (SoundParameters) {
        .hit_sound = "wood_hit"
    });

    return i;
}


Entity create_table(Vector3 position, float yaw) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
        .yaw = yaw
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "table",
        .texture_filename = "wood",
        .material_filename = "concrete"
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_AABB,
        .group = GROUP_WALLS,
        .width = 3.0f,
        .height = 2.1f,
        .depth = 1.5f
    });

    return i;
}


Entity create_book(Vector3 position, float yaw, float thickness, float height) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
        .yaw = yaw,
        .scale = vec3(1.0f, height / 0.5f, thickness / 0.2f)
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "book",
        .texture_filename = "book",
        .material_filename = "concrete"
    });
    ColliderComponent_add(i, (ColliderParameters) {
        .type = COLLIDER_CUBOID,
        .group = GROUP_PROPS,
        .width = 0.4f,
        .height = 0.5f,
        .depth = 0.2f
    });
    RigidBodyComponent_add(i, (RigidBodyParameters) {
        .mass = 1.0f,
        .friction = 1.0f,
        .bounce = 0.2f,
    });
    SoundComponent_add(i, (SoundParameters) {
        .hit_sound = "wood_hit"
    });

    return i;
}


void create_bookcase(Vector3 position, float yaw) {
    float width = 1.5f;
    float depth = 0.5f;
    float height = 2.5f;
    float shelf_thickness = 0.05f;

    Entity parent = create_entity();
    TransformComponent_add(parent, (TransformParameters) {
        .position = position,
        .yaw = yaw
    });

    Matrix4 transform = get_transform(parent);

    for (int i = -1; i < 2; i += 2) {
        Entity side = create_entity();
        TransformComponent_add(side, (TransformParameters) {
            .position = vec3(i * 0.5f * width, 0.5f * height, 1.0f),
            .scale = vec3(shelf_thickness, height, depth),
            .parent = parent
        });
        MeshComponent_add(side, (MeshParameters) {
            .mesh_filename = "cube",
            .texture_filename = "wood",
            .material_filename = "concrete"
        });
        ColliderComponent_add(side, (ColliderParameters) {
            .type = COLLIDER_CUBOID,
            .group = GROUP_WALLS
        });
    }

    int shelves = 4;
    float offset = 0.5f;
    float shelf_height = (height - offset) / (shelves - 1);
    for (int i = 0; i < 4; i++) {
        Entity shelf = create_entity();
        TransformComponent_add(shelf, (TransformParameters) {
            .position = vec3(0.0f, i * shelf_height + offset - 0.5f * shelf_thickness, 1.0f),
            .scale = vec3(width - shelf_thickness, shelf_thickness, depth),
            .parent = parent
        });
        MeshComponent_add(shelf, (MeshParameters) {
            .mesh_filename = "cube",
            .texture_filename = "wood",
            .material_filename = "concrete"
        });
        ColliderComponent_add(shelf, (ColliderParameters) {
            .type = COLLIDER_CUBOID,
            .group = GROUP_WALLS
        });

        if (i == 3) {
            break;
        }

        float accum_width = 0.0f;
        float dir = sign(randf(-1.0f, 1.0f));

        for (int j = 0; j < randi(1, 5); j++) {
            float w = randf(0.1f, 0.2f);
            float h = randf(0.3f, 0.5f);

            float x = accum_width + 0.5f * (w - width + shelf_thickness);
            float y = i * shelf_height + offset + 0.5f * shelf_thickness + 0.5f * h;
            Vector4 pos = map4(transform, vec4(x * dir, y, 1.0f, 1.0f));
            create_book(
                vec4_xyz(pos),
                yaw - 90.0f,
                w,
                h
            );
            accum_width += w;
        }
    }

    Entity force = create_entity();
    TransformComponent_add(force, (TransformParameters) {
        .position = vec3(0.0f, 0.5f * height, 1.0f),
        .parent = parent
    });
    ColliderComponent_add(force, (ColliderParameters) {
        .type = COLLIDER_CUBOID,
        .group = GROUP_NONE,
        .width = width,
        .height = height,
        .depth = depth
    });
    ForceComponent_add(force, (ForceParameters) {
        .direction = vec3(0.0f, 0.0f, -1.0f),
        .magnitude = 10.0f,
        .disabled = true,
        .duration = 0.5f
    });

    Entity trigger = create_entity();
    TransformComponent_add(trigger, (TransformParameters) {
        .position = position,
    });
    TriggerComponent_add(trigger, (TriggerParameters) {
        .type = TRIGGER_MANUAL,
        .target_entity = force,
        .on_enter = enable_force
    });
}


void create_tree(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = position,
        .rotation = random_y_rotation(),
        .scale = diag3(randf(0.5f, 1.0f))
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "tree",
        .texture_filename = "bark",
        .material_filename = "concrete",
    });
}


Entity create_shrub(Vector3 position) {
    Entity i = create_entity();
    float scale = randf(1.0f, 2.0f);
    TransformComponent_add(i, (TransformParameters) {
        .position = add3(position, vec3(0.0f, 0.5f, 0.0f)),
        .scale = diag3(scale)
    });
    SpriteComponent_add(i, (SpriteParameters) {
        .texture_filename = "shrub",
    });
    return i;
}


Entity create_blood(Vector3 position, bool hidden) {
    Entity i = create_entity();
    Quaternion rotate_x = axis_angle_to_quaternion(vec3(1.0f, 0.0f, 0.0f), to_radians(-90.0f));
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + 1e-3f, position.z),
        .rotation = quaternion_mult(random_y_rotation(), rotate_x),
        .scale = diag3(randf(1.0f, 2.0f))
    });
    MeshParameters params = {
        .mesh_filename = "quad",
        .texture_filename = "blood",
        .material_filename = "glass",
        .visibility = hidden ? VISIBILITY_UV : VISIBILITY_ALL
    };
    if (hidden) {
        strcpy(params.material_filename, "hidden");
    }
    MeshComponent_add(i, params);

    if (hidden) {
        return i;
    }

    Entity j = create_entity();
    TransformComponent_add(j, (TransformParameters) {
        .position = vec3(0.0f, 0.0f, 1e-3f),
        .parent = i
    });
    MeshComponent_add(j, (MeshParameters) {
        .mesh_filename = "quad",
        .texture_filename = "blood",
        .material_filename = "hidden",
        .visibility = VISIBILITY_UV
    });

    return i;
}


Entity create_dust_particles(Vector3 position, float width, float depth, float height) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) { .position = position });
    EmitterComponent_add(i, (EmitterParameters) {
        .spawn_rate = 5.0f,
        .position_variance = vec3(width, height, depth),
        .velocity = zeros3(),
        .velocity_variance = diag3(0.1f),
        .particle_type_name = "dust"
    });

    return i;
}


Entity create_fire(Vector3 position, float size) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) { .position = position });
    EmitterComponent_add(i, (EmitterParameters) {
        .spawn_rate = 10.0f,
        .velocity_variance = diag3(0.1f),
        .particle_type_name = "fire",
        .scale = size
    });

    return i;
}


Entity create_blood_dripper(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) { .position = position });
    EmitterComponent_add(i, (EmitterParameters) {
        .spawn_rate = randf(1.0f, 1.1f),
        .velocity = vec3(0.0f, -0.1f, 0.0f),
        .position_variance = vec3(0.1f, 0.0f, 0.1f),
        .particle_type_name = "blood"
    });

    return i;
}
