#define _USE_MATH_DEFINES

#include "camera.h"
#include "raycast.h"
#include "systems/physics.h"
#include "systems/player.h"

#include <stdio.h>
#include <systems/sound.h>

#include "scene.h"


void update_players(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        PlayerComponent* player = get_component(i, COMPONENT_PLAYER);
        if (!player) continue;

        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Vector2 velocity = vec2(rb->velocity.x, rb->velocity.z);
        float speed = norm2(velocity);

        if (rb->on_ground) {
            if (player->footstep_timer > 0.0f) {
                player->footstep_timer -= time_step * speed;
            } else {
                player->footstep_timer = 1.0f;
                add_sound(i, "footstep", 0.1f, 1.0f);
            }
        } else {
            player->footstep_timer = 0.0f;
        }
    }
}


void input_players() {
    for (int i = 0; i < scene->components->entities; i++) {
        PlayerComponent* player = get_component(i, COMPONENT_PLAYER);
        if (!player) continue;

        ControllerComponent* controller = get_component(i, COMPONENT_CONTROLLER);

        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);
        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        Entity camera = trans->children->head->value;
        CameraComponent* cam = get_component(camera, COMPONENT_CAMERA);

        TransformComponent* trans_cam = get_component(camera, COMPONENT_TRANSFORM);
        trans_cam->position.y = player->head_height + player->view_bobbing * sinf(0.5f * player->footstep_timer * 2.0f * M_PI);

        if (!player->examining) {
            player->yaw += controller->controller.right_stick.x;
            player->pitch += controller->controller.right_stick.y;
            player->pitch = clamp(player->pitch, -89.0f, 89.0f);

            Vector2 v = controller->controller.left_stick;
            Vector3 velocity = vec3(v.x, 0.0f, -v.y);
            velocity = mul3(3.0f, normalized3(velocity));

            Matrix3 rot = quaternion_to_rotation_matrix(trans->rotation);
            velocity = map3(rot, velocity);

            rb->velocity.x = velocity.x;
            rb->velocity.z = velocity.z;

            Quaternion q_yaw = axis_angle_to_quaternion(vec3(0.0f, 1.0f, 0.0f), to_radians(player->yaw));
            Quaternion q_pitch = axis_angle_to_quaternion(vec3(1.0f, 0.0f, 0.0f), to_radians(player->pitch));

            trans->rotation = q_yaw;

            // Camera only moves in pitch direction
            TransformComponent* camera_trans = get_component(camera, COMPONENT_TRANSFORM);
            camera_trans->rotation = q_pitch;
        } else {
            rb->velocity.x = 0.0f;
            rb->velocity.z = 0.0f;
        }

        if (controller->controller.buttons_pressed[BUTTON_A]) {
            if (rb->on_ground) {
                rb->velocity.y += 4.0f;
            }
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
                    cam->dof_enabled = false;
                }
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
                        cam->dof_enabled = true;
                        cam->focal_distance = 1.0f;
                        cam->focal_range = 1.0f;
                    }
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
