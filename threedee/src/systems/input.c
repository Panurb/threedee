#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "app.h"
#include "../../include/systems/input.h"

#include <raycast.h>
#include <scene.h>
#include <systems/physics.h>

#include "util.h"
#include "component.h"
#include "camera.h"
#include "settings.h"


char* KEY_NAMES[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", 
    "U", "V", "W", "X", "Y", "Z", "Num0", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8", "Num9", 
    "Escape", "LControl", "LShift", "LAlt", "LSystem", "RControl", "RShift", "RAlt", "RSystem", "Menu", "LBracket", 
    "RBracket", "Semicolon", "Comma", "Period", "Quote", "Slash", "Backslash", "Tilde", "Equal", "Hyphen", "Space", 
    "Enter", "Backspace", "Tab", "PageUp", "PageDown", "End", "Home", "Insert", "Delete", "Add", "Subtract", "Multiply", 
    "Divide", "Left", "Right", "Up", "Down", "Numpad0", "Numpad1", "Numpad2", "Numpad3", "Numpad4", "Numpad5", 
    "Numpad6", "Numpad7", "Numpad8", "Numpad9", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", 
    "F12", "F13", "F14", "F15", "Pause"};


char* MOUSE_NAMES[] = {"Mouse left", "Mouse middle", "Mouse right", "Mouse extra 1", "Mouse extra 2"};


char* LETTERS[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", 
    "u", "v", "w", "x", "y", "z"};


char* BUTTON_NAMES[] = {
    "A",
    "B",
    "X",
    "Y",
    "LB",
    "RB",
    "START",
    "BACK",
    "LT",
    "RT",
    "L",
    "R"
};


char* ACTIONS[] = {
    "MOVE_UP",
    "MOVE_LEFT",
    "MOVE_DOWN",
    "MOVE_RIGHT",
    "ATTACK",
    "ENTER",
    "PICK_UP",
    "RELOAD",
    "ATTACHMENT",
    "INVENTORY",
    "AMMO"
};

int ACTIONS_SIZE = LENGTH(ACTIONS);


char* ACTION_BUTTONS_XBOX[] = {
    "",
    "",
    "",
    "",
    "RT",
    "A",
    "RB",
    "X",
    "Y",
    "LT",
    "LB"
};

static Vector2 mouse_motion = {0.0f, 0.0f};


const char* key_to_string(SDL_Scancode key) {
    return SDL_GetScancodeName(key);
}

char* mouse_to_string(int button) {
    return (1 <= button && button < LENGTH(MOUSE_NAMES)) ? MOUSE_NAMES[button - 1] : "unknown";
}


char* keybind_to_string(Keybind keybind) {
    if (keybind.device == DEVICE_KEYBOARD) {
        return key_to_string(keybind.key);
    } else if (keybind.device == DEVICE_MOUSE) {
        return mouse_to_string(keybind.key);
    } else {
        return "unknown";
    }
}


Keybind string_to_keybind(String string) {
    Keybind keybind = {DEVICE_UNBOUND, 0};

    for (int i = 0; i < LENGTH(MOUSE_NAMES); i++) {
        if (strcmp(string, MOUSE_NAMES[i]) == 0) {
            keybind.device = DEVICE_MOUSE;
            keybind.key = i + 1;
            return keybind;
        }
    }

    SDL_Scancode scancode = SDL_GetScancodeFromName(string);

    if (scancode != SDL_SCANCODE_UNKNOWN) {
        keybind.device = DEVICE_KEYBOARD;
        keybind.key = scancode;
        return keybind;
    }

    return keybind;
}


char* action_to_keybind(char* action) {
    for (int i = 0; i < LENGTH(ACTIONS); i++) {
        if (strcmp(action, ACTIONS[i]) == 0) {
            return keybind_to_string(game_settings.keybinds[i]);
        }
    }

    if (strcmp(action, "AIM") == 0) {
        return "Mouse";
    }

    return action;
}


bool keybind_pressed(PlayerAction i) {
    Keybind keybind = game_settings.keybinds[i];
    if (keybind.device == DEVICE_KEYBOARD) {
        return SDL_GetKeyboardState(NULL)[keybind.key];
    } else if (keybind.device == DEVICE_MOUSE) {
        return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(keybind.key);
    } else {
        return false;
    }
}


void replace_actions(String output, String input) {
    output[0] = '\0';
    char* start = strchr(input, '[');
    char* end = input;
    while (start) {
        strncat(output, end, start - end + 1);
        start++;

        end = strchr(start, ']');
        if (end == NULL) {
            strcat(output, start);
            return;
        }
        
        *end = '\0';
        strcat(output, action_to_keybind(start));
        *end = ']';

        start = end + 1;
        start = strchr(start, '[');
    }

    strcat(output, end);
}


Vector2 get_mouse_position(int camera) {
    // int x;
    // int y;
    // SDL_GetMouseState(&x, &y);
    // return screen_to_world(camera, (Vector2) {x, y});
    return zeros2();
}


void update_controller(Entity entity) {
    ControllerComponent* player = get_component(entity, COMPONENT_CONTROLLER);
    int joystick = player->controller.joystick;

    Vector2 left_stick = zeros2();
    Vector2 right_stick = zeros2();
    if (player->controller.joystick == -1) {
        if (keybind_pressed(ACTION_LEFT)) {
            left_stick.x -= 1.0f;
        }
        if (keybind_pressed(ACTION_RIGHT)) {
            left_stick.x += 1.0f;
        }
        if (keybind_pressed(ACTION_DOWN)) {
            left_stick.y -= 1.0f;
        }
        if (keybind_pressed(ACTION_UP)) {
            left_stick.y += 1.0f;
        }
        player->controller.left_stick = normalized2(left_stick);

        player->controller.right_stick = mouse_motion;
        mouse_motion = zeros2();
        player->controller.left_trigger = keybind_pressed(ACTION_ATTACK) ? 1.0f : 0.0f;
        player->controller.right_trigger = keybind_pressed(ACTION_PICKUP) ? 1.0f : 0.0f;

        for (ControllerButton b = BUTTON_A; b <= BUTTON_R; b++) {
            bool down = false;
            switch (b) {
                case BUTTON_A:
                    down = keybind_pressed(ACTION_JUMP);
                    break;
                case BUTTON_B:
                    break;
                case BUTTON_X:
                    down = keybind_pressed(ACTION_RELOAD);
                    break;
                case BUTTON_Y:
                    down = keybind_pressed(ACTION_ATTACHMENT);
                    break;
                case BUTTON_LB:
                    down = keybind_pressed(ACTION_AMMO);
                    break;
                case BUTTON_RB:
                    down = keybind_pressed(ACTION_PICKUP);
                    break;
                case BUTTON_START:
                    break;
                case BUTTON_BACK:
                    break;
                case BUTTON_LT:
                    down = keybind_pressed(ACTION_INVENTORY);
                    break;
                case BUTTON_RT:
                    down = keybind_pressed(ACTION_ATTACK);
                    break;
                case BUTTON_L:
                    break;
                case BUTTON_R:
                    break;
            }

            player->controller.buttons_pressed[b] = (down && !player->controller.buttons_down[b]);
            player->controller.buttons_released[b] = (!down && player->controller.buttons_down[b]);
            player->controller.buttons_down[b] = down;
        }
    } else {
        SDL_Gamepad* controller = app.controllers[joystick];

        left_stick.x = map_to_range(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTX),
            SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, -1.0f, 1.0f);
        left_stick.y = -map_to_range(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFTY),
            SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, -1.0f, 1.0f);
        if (fabsf(left_stick.x) < 0.05f) {
            left_stick.x = 0.0f;
        }
        if (fabsf(left_stick.y) < 0.05f) {
            left_stick.y = 0.0f;
        }
        player->controller.left_stick = left_stick;

        right_stick.x = map_to_range(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTX),
            SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, -1.0f, 1.0f);
        right_stick.y = -map_to_range(SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHTY),
            SDL_JOYSTICK_AXIS_MIN, SDL_JOYSTICK_AXIS_MAX, -1.0f, 1.0f);
        if (norm2(right_stick) < 0.25f) {
            right_stick = zeros2();
        }
        if (norm2(right_stick) > 1.0f) {
            right_stick = normalized2(right_stick);
        }
        player->controller.right_stick = right_stick;

        float trigger = 0.01f * SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        if (fabsf(trigger) < 0.1f) {
            trigger = 0.0f;
        }
        player->controller.left_trigger = trigger;

        trigger = 0.01f * SDL_GetGamepadAxis(controller, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
        if (fabsf(trigger) < 0.1f) {
            trigger = 0.0f;
        }
        player->controller.right_trigger = trigger;

        // player->controller.dpad.x = 0.01f * SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        // player->controller.dpad.y = 0.01f * SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);

        for (int b = BUTTON_A; b <= BUTTON_R; b++) {
            SDL_GamepadButton button = player->controller.buttons[b];
            bool down = SDL_GetGamepadButton(controller, player->controller.buttons[b]);
            if (b == BUTTON_LT) {
                down = (player->controller.left_trigger > 0.5f);
            } else if (b == BUTTON_RT) {
                down = (player->controller.right_trigger > 0.5f);
            }

            player->controller.buttons_pressed[b] = (down && !player->controller.buttons_down[b]);
            player->controller.buttons_released[b] = (!down && player->controller.buttons_down[b]);
            player->controller.buttons_down[b] = down;
        }
    }
}


void input_players() {
    static Entity grabbed_entity = NULL_ENTITY;

    for (int i = 0; i < scene->components->entities; i++) {
        ControllerComponent* controller = get_component(i, COMPONENT_CONTROLLER);
        if (!controller) continue;

        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);
        RigidBodyComponent* rb = get_component(i, COMPONENT_RIGIDBODY);

        update_controller(i);

        Vector2 v = controller->controller.left_stick;
        Vector3 velocity = vec3(v.x, 0.0f, -v.y);
        velocity = mult3(3.0f, normalized3(velocity));

        Matrix3 rot = quaternion_to_rotation_matrix(trans->rotation);
        velocity = matrix3_map(rot, velocity);

        rb->velocity.x = velocity.x;
        rb->velocity.z = velocity.z;

        controller->yaw += controller->controller.right_stick.x;
        controller->pitch += controller->controller.right_stick.y;
        controller->pitch = clamp(controller->pitch, -89.0f, 89.0f);

        Quaternion q_yaw = axis_angle_to_quaternion(vec3(0.0f, 1.0f, 0.0f), to_radians(controller->yaw));
        Quaternion q_pitch = axis_angle_to_quaternion(vec3(1.0f, 0.0f, 0.0f), to_radians(controller->pitch));

        trans->rotation = q_yaw;

        // Camera only moves in pitch direction
        Entity camera = trans->children->head->value;
        TransformComponent* camera_trans = get_component(camera, COMPONENT_TRANSFORM);
        camera_trans->rotation = q_pitch;

        if (controller->controller.buttons_pressed[BUTTON_A]) {
            rb->velocity.y += 4.0f;
        }

        Matrix4 camera_transform = get_transform(scene->camera);
        Matrix4 inv_camera_transform = transform_inverse(camera_transform);
        if (controller->controller.buttons_pressed[BUTTON_RT]) {
            if (grabbed_entity != NULL_ENTITY) {
                Vector3 dir = look_direction(scene->camera);
                apply_impulse(grabbed_entity, get_position(grabbed_entity), mult3(10.0f, dir));
                // remove_parent(grabbed_entity);
                // set_transform(grabbed_entity, matrix4_mult(camera_transform, get_transform(grabbed_entity)));
                RigidBodyComponent* grabbed_rb = get_component(grabbed_entity, COMPONENT_RIGIDBODY);
                grabbed_rb->gravity_scale = 1.0f;
                grabbed_entity = NULL_ENTITY;
            } else {
                Vector3 dir = look_direction(scene->camera);
                Ray ray = { get_position(scene->camera), dir };
                Hit hit = raycast(ray, GROUP_PROPS);
                if (hit.entity != NULL_ENTITY && hit.distance < 3.0f) {
                    grabbed_entity = hit.entity;
                    // set_transform(grabbed_entity, matrix4_mult(inv_camera_transform, get_transform(grabbed_entity)));
                    // add_child(scene->camera, grabbed_entity);
                    RigidBodyComponent* grabbed_rb = get_component(grabbed_entity, COMPONENT_RIGIDBODY);
                    grabbed_rb->gravity_scale = 0.0f;
                    grabbed_rb->velocity = zeros3();
                    grabbed_rb->angular_velocity = zeros3();
                }
            }
        }

        if (grabbed_entity != NULL_ENTITY) {
            Vector3 target_position = sum3(get_position(camera), mult3(2.0f, look_direction(camera)));
            Quaternion target_rotation = get_rotation(camera);

            // Update grabbed entity position to camera position
            TransformComponent* trans = get_component(grabbed_entity, COMPONENT_TRANSFORM);
            RigidBodyComponent* rb = get_component(grabbed_entity, COMPONENT_RIGIDBODY);
            Vector3 delta = diff3(target_position, get_position(grabbed_entity));
            rb->velocity = mult3(10.0f, delta);
            // trans->rotation = target_rotation;
        }
    }
}


void input_game(SDL_Event sdl_event) {
    if (sdl_event.type == SDL_EVENT_KEY_DOWN && sdl_event.key.key == SDLK_ESCAPE) {
        app.quit = true;
    }

    if (sdl_event.type == SDL_EVENT_KEY_DOWN && sdl_event.key.repeat == 0) {
        if (sdl_event.key.key == SDLK_F1) {
            if (game_settings.debug) {
                app.debug_level = (app.debug_level + 1) % 4;
            }
        }
    }

    switch (app.state) {
        case STATE_GAME:

        default:
            break;
    }


    float sens = game_settings.mouse_sensitivity / 10.0f;

    if (sdl_event.type == SDL_EVENT_MOUSE_MOTION) {
        mouse_motion.x -= sdl_event.motion.xrel * sens;
        mouse_motion.y -= sdl_event.motion.yrel * sens;
    }
}
