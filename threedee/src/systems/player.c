#define _USE_MATH_DEFINES

#include "camera.h"
#include "raycast.h"
#include "systems/physics.h"
#include "systems/player.h"

#include <stdio.h>
#include <systems/sound.h>

#include "scene.h"


Entity create_player(Vector3 position) {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(position.x, position.y + 1.0f, position.z),
    });
    RigidBodyComponent* rb = RigidBodyComponent_add(i, (RigidBodyParameters) {
        .mass = 80.0f,
        .friction = 0.0f,
        .bounce = 0.0f,
        .axis_lock.rotation = true,
        .dont_sleep = true
    });
    rb->linear_damping = 0.99f;
    // MeshComponent_add(i, (MeshParameters) {
    //     .mesh_filename = "sphere",
    //     .texture_filename = "black",
    //     .material_filename = "plastic",
    // });
    ColliderComponent_add(i,
        (ColliderParameters) {
            .type = COLLIDER_CAPSULE,
            .group = GROUP_PLAYERS,
            .radius = 0.4f,
            .height = 1.0f
        }
    );
    ControllerComponent_add(i, -1);
    PlayerComponent* player = PlayerComponent_add(i);
    SoundComponent_add(i, (SoundParameters) {});
    WaypointComponent_add(i);

    Entity cam = create_entity();
    TransformComponent_add(cam, (TransformParameters) {
        .position = vec3(0.0f, player->head_height, 0.0f),
    });
    CameraComponent_add(cam,
        (Resolution) { game_settings.width, game_settings.height },
        to_radians(game_settings.fov)
    );
    add_child(i, cam);

    Entity j = create_entity();
    TransformComponent_add(j, (TransformParameters) {
        .position = vec3(0.0f, -0.5f, 0.1f)
    });
    MeshComponent_add(j, (MeshParameters) {
        .mesh_filename = "flashlight",
        .texture_filename = "black",
        .material_filename = "plastic",
        .visibility = VISIBILITY_ALL
    });
    Entity light = create_entity();
    TransformComponent_add(light, (TransformParameters) {
        .position = vec3(0.0f, 0.0f, -0.1f),
        .parent = j
    });
    LightComponent_add(light, (LightParameters) {
        .disabled = false,
        .shape = LIGHT_SPOT,
        .color = COLOR_WHITE,
        .fov = 50.0f,
        .visibility_mask = VISIBILITY_NORMAL
    });
    ArrayList_add(player->inventory, &j);

    Entity k = create_entity();
    TransformComponent_add(k, (TransformParameters) {
        .position = vec3(0.0f, -0.5f, 0.15f),
        .scale = diag3(0.5f),
    });
    MeshComponent_add(k, (MeshParameters) {
        .mesh_filename = "flashlight",
        .texture_filename = "black",
        .material_filename = "plastic",
        .visibility = VISIBILITY_ALL,
        .invisible = true
    });
    light = create_entity();
    TransformComponent_add(light, (TransformParameters) {
        .position = vec3(0.0f, 0.0f, -0.1f),
        .parent = k
    });
    LightComponent_add(light, (LightParameters) {
        .disabled = true,
        .shape = LIGHT_SPOT,
        .color = COLOR_UV,
        .fov = 70.0f,
        .range = 4.0f,
        .visibility_mask = VISIBILITY_UV
    });
    ArrayList_add(player->inventory, &k);

    player->selected_item = 0;

    return i;
}


float get_bobbing_height(Entity player, float offset) {
    PlayerComponent* p = get_component(player, COMPONENT_PLAYER);
    if (!p) return 0.0f;

    float scale = 1.0f;
    if (p->sprinting) {
        scale = 3.0f;
    }

    return p->view_bobbing * scale * sinf(0.5f * (p->footstep_timer + offset) * 2.0f * M_PI);
}


Entity get_current_item(Entity player) {
    PlayerComponent* p = get_component(player, COMPONENT_PLAYER);
    if (!p || p->inventory->size == 0) return NULL_ENTITY;
    Entity i = *(Entity*)ArrayList_get(p->inventory, p->selected_item);
    return i;
}


bool in_player_view(Entity player, Entity entity, float distance, float roi) {
    Vector3 point = get_position(entity);
    Entity camera = get_player_camera(player);
    Vector3 cam_pos = get_position(camera);
    Vector3 to_point = sub3(point, cam_pos);
    Vector3 forward = look_direction(camera);
    float dist = norm3(to_point);

    if (dist > distance) {
        return false;
    }

    MeshComponent* mesh = get_component(entity, COMPONENT_MESH);
    if (mesh) {
        Entity item = get_current_item(player);
        LightComponent* light = get_component(get_children(item)->head->value, COMPONENT_LIGHT);
        if (light && mesh->visibility == VISIBILITY_UV) {
            if (light->range < dist) {
                return false;
            }
            if (!(mesh->visibility & light->visibility_mask)) {
                return false;
            }
        }
    }

    to_point = div3(dist, to_point);

    float angle = acosf(dot3(to_point, forward));
    CameraComponent* cam = get_component(camera, COMPONENT_CAMERA);
    float fov = cam->fov * 0.5f;

    if (angle > roi * fov) {
        return false;
    }

    Hit hit = raycast(
        (Ray) {
            cam_pos,
            to_point,
            distance
        },
        GROUP_WALLS | GROUP_PLAYERS
    );

    if (hit.entity == entity || hit.entity == NULL_ENTITY) {
        return true;
    }

    return false;
}


void update_players(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        PlayerComponent* player = get_component(i, COMPONENT_PLAYER);
        if (!player) continue;

        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Vector2 velocity = vec2(rb->velocity.x, rb->velocity.z);
        float speed = norm2(velocity);

        if (player->sprinting) {
            player->sprint_timer = fmaxf(player->sprint_timer - time_step, 0.0f);
        } else {
            player->sprint_timer = fminf(player->sprint_timer + 0.5f * time_step, player->max_sprint);
        }

        if (rb->on_ground) {
            if (player->footstep_timer > 0.0f) {
                player->footstep_timer -= time_step * speed * (1.0f - 0.5f * player->sprinting);
            } else {
                player->footstep_timer = player->footstep_interval;
                add_sound(i, "footstep", 0.1f, 1.0f);
                player->foot = (player->foot == FOOT_LEFT) ? FOOT_RIGHT : FOOT_LEFT;
            }
        } else {
            player->footstep_timer = 0.0f;
            player->foot = FOOT_BOTH;
        }

        Entity camera = get_player_camera(i);
        TransformComponent* trans_cam = get_component(camera, COMPONENT_TRANSFORM);

        Quaternion q_yaw = axis_angle_to_quaternion(vec3_up(), to_radians(player->yaw));
        Quaternion q_pitch = axis_angle_to_quaternion(vec3_right(), to_radians(player->pitch));

        trans->rotation = q_yaw;

        // Camera only moves in pitch direction
        trans_cam->rotation = q_pitch;

        trans_cam->position.y = player->head_height + get_bobbing_height(i, 0.0f);

        Vector3 position = get_position(camera);
        Vector3 forward = look_direction(camera);
        Vector3 right = normalized3(cross(forward, vec3_up()));
        Vector3 up = cross(right, forward);

        Vector3 item_pos = add3(position, mul3(0.12f, forward));
        float item_x = 0.25f * player->foot * get_bobbing_height(i, 0.5f);
        if (player->sprinting) {
            item_x *= 2.0f;
        }
        float item_y = -0.1f + 0.15f * get_bobbing_height(i, 0.0f);

        item_pos = add3(item_pos, mul3(0.15f + item_x, right));
        item_pos = add3(item_pos, mul3(item_y, up));

        Vector3 item_dir = forward;
        if (player->sprinting) {
            item_dir = sub3(item_dir, mul3(0.75f, up));
            item_dir = sub3(item_dir, mul3(-5.0f * item_x, right));
        }
        Quaternion item_rotation = quaternion_from_forward(item_dir, vec3_up());

        for (int j = 0; j < player->inventory->size; j++) {
            Entity item = *(Entity*)ArrayList_get(player->inventory, j);

            TransformComponent* trans_item = get_component(item, COMPONENT_TRANSFORM);

            trans_item->position = item_pos;
            trans_item->rotation = slerp(trans_item->rotation, item_rotation, 0.1f);
            // trans_item->rotation = item_rotation;
        }

        if (player->look_target != NULL_ENTITY) {
            Vector3 target_pos = get_position(player->look_target);
            float y = target_pos.y - position.y;
            float d = norm3(sub3(target_pos, position));
            float target_pitch = asinf(y / d);

            float target_yaw = atan2f(target_pos.x - position.x, target_pos.z - position.z) + M_PI;

            player->yaw = to_degrees(lerp_angle(to_radians(player->yaw), target_yaw, 0.1f));
            player->pitch =  to_degrees(lerp_angle(to_radians(player->pitch), target_pitch, 0.1f));
        }

        if (player->look_timer > 0.0f) {
            player->look_timer -= time_step;
        } else {
            player->look_timer = 0.0f;
            player->look_target = NULL_ENTITY;
        }
    }
}


void toggle_visibility(Entity entity) {
    if (entity == NULL_ENTITY) return;

    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);

    MeshComponent* mesh = get_component(entity, COMPONENT_MESH);
    if (mesh) {
        mesh->visible = !mesh->visible;
    }
    LightComponent* light = get_component(entity, COMPONENT_LIGHT);
    if (light) {
        light->disabled = !light->disabled;
    }

    ListNode* node;
    FOREACH(node, trans->children) {
        toggle_visibility(node->value);
    }
}


float get_player_speed(Entity player) {
    PlayerComponent* p = get_component(player, COMPONENT_PLAYER);
    if (!p) return 0.0f;
    return p->sprinting ? p->sprint_speed : p->walk_speed;
}


void input_players() {
    for (int i = 0; i < scene->components->entities; i++) {
        PlayerComponent* player = get_component(i, COMPONENT_PLAYER);
        if (!player) continue;

        ControllerComponent* controller = get_component(i, COMPONENT_CONTROLLER);

        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);
        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Entity camera = get_player_camera(i);
        CameraComponent* cam = get_component(camera, COMPONENT_CAMERA);

        if (controller->controller.buttons_down[BUTTON_X] && norm2(controller->controller.left_stick) > 0.0f) {
            if (player->sprint_timer > 0.5f * player->max_sprint) {
                player->sprinting = true;
            }
        }
        if (controller->controller.buttons_released[BUTTON_X] || player->sprint_timer <= 0.0f) {
            player->sprinting = false;
        }

        if (!player->examining) {
            player->yaw += controller->controller.right_stick.x;
            player->pitch += controller->controller.right_stick.y;
            player->pitch = clamp(player->pitch, -89.0f, 89.0f);

            if (rb->on_ground) {
                Vector2 v = controller->controller.left_stick;
                Vector3 velocity = vec3(v.x, 0.0f, -v.y);
                velocity = mul3(get_player_speed(i), normalized3(velocity));

                Matrix3 rot = quaternion_to_rotation_matrix(trans->rotation);
                velocity = map3(rot, velocity);

                rb->velocity.x = velocity.x;
                rb->velocity.z = velocity.z;
            }
        } else {
            rb->velocity.x = 0.0f;
            rb->velocity.z = 0.0f;
        }

        if (controller->controller.buttons_pressed[BUTTON_A]) {
            if (rb->on_ground) {
                rb->velocity.y += 3.0f;
            }
        }

        int item_switch = 0;

        if (controller->controller.buttons_pressed[BUTTON_RB]) {
            item_switch = 1;
        }

        if (controller->controller.buttons_pressed[BUTTON_LB]) {
            item_switch = -1;
        }

        if (item_switch) {
            Vector3 camera_pos = get_position(camera);
            Axes camera_axes = get_axes(camera);

            toggle_visibility(get_current_item(i));
            player->selected_item = (player->selected_item + item_switch) % player->inventory->size;
            look_at(get_current_item(i), add3(camera_pos, add3(camera_axes.down, camera_axes.forward)));
            toggle_visibility(get_current_item(i));
        }

        if (controller->controller.buttons_pressed[BUTTON_RT]) {
            if (player->grabbed_entity != NULL_ENTITY) {
                Vector3 dir = look_direction(scene->camera);
                apply_impulse(player->grabbed_entity, get_position(player->grabbed_entity), mul3(10.0f, dir));
                RigidBodyComponent* grabbed_rb = get_component(player->grabbed_entity, COMPONENT_RIGIDBODY);
                if (grabbed_rb) {
                    grabbed_rb->gravity_scale = 1.0f;
                } else {
                    player->examining = false;
                    trans = get_component(player->grabbed_entity, COMPONENT_TRANSFORM);
                    trans->position = player->grabbed_position;
                    trans->rotation = player->grabbed_rotation;
                }
                cam->dof_enabled = false;
                player->grabbed_entity = NULL_ENTITY;
            } else {
                Vector3 dir = look_direction(scene->camera);
                Ray ray = { get_position(scene->camera), dir };
                Hit hit = raycast(ray, GROUP_PROPS | GROUP_ITEMS);
                if (hit.entity != NULL_ENTITY && hit.distance < 3.0f) {
                    player->grabbed_entity = hit.entity;
                    RigidBodyComponent* grabbed_rb = get_component(player->grabbed_entity, COMPONENT_RIGIDBODY);
                    if (grabbed_rb) {
                        grabbed_rb->gravity_scale = 0.0f;
                        grabbed_rb->velocity = zeros3();
                        grabbed_rb->angular_velocity = zeros3();
                    } else {
                        player->examining = true;
                        player->examine_yaw = player->yaw;
                        player->grabbed_position = get_position(player->grabbed_entity);
                        player->grabbed_rotation = get_rotation(player->grabbed_entity);
                    }
                    cam->dof_enabled = true;
                    cam->focal_distance = 1.0f;
                    cam->focal_range = 1.0f;
                }
            }
        }

        if (player->grabbed_entity != NULL_ENTITY) {
            Vector3 target_position = add3(get_position(camera), mul3(2.0f, look_direction(camera)));
            Quaternion target_rotation = get_rotation(camera);

            // Update grabbed entity position to camera position
            trans = get_component(player->grabbed_entity, COMPONENT_TRANSFORM);
            RigidBodyComponent* rb = get_component(player->grabbed_entity, COMPONENT_RIGIDBODY);
            if (rb) {
                Vector3 delta = sub3(target_position, get_position(player->grabbed_entity));
                rb->velocity = mul3(10.0f, delta);
                rb->asleep = false;
            } else {
                player->examine_yaw -= controller->controller.right_stick.x;

                target_position = add3(get_position(camera), mul3(1.0f, look_direction(camera)));
                trans->position = lerp3(get_position(player->grabbed_entity), target_position, 0.5f);

                Quaternion q_yaw = axis_angle_to_quaternion(vec3(0.0f, 1.0f, 0.0f), to_radians(player->examine_yaw));
                trans->rotation = slerp(get_rotation(player->grabbed_entity), q_yaw, 0.5f);
            }
        }
    }
}


Entity get_player_camera(Entity player) {
    return get_children(player)->head->value;
}


void player_look(Entity trigger, Entity entity) {
    PlayerComponent* player = get_component(entity, COMPONENT_PLAYER);
    if (!player) return;

    player->look_target = trigger;
    player->look_timer = 0.5f;

    add_sound(entity, "discover", 1.0f, 1.0f);
}
