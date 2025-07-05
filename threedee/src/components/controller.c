#include <SDL3/SDL_gamepad.h>

#include "components/controller.h"
#include "scene.h"
#include "util.h"


ControllerComponent* ControllerComponent_add(Entity entity, int joystick) {
    ControllerComponent* input = malloc(sizeof(ControllerComponent));

    input->controller.joystick = joystick;
    int buttons[12] = {
        SDL_GAMEPAD_BUTTON_SOUTH,
        SDL_GAMEPAD_BUTTON_EAST,
        SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_NORTH,
        SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
        SDL_GAMEPAD_BUTTON_START,
        SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_LEFT_STICK,
        SDL_GAMEPAD_BUTTON_RIGHT_STICK,
        SDL_GAMEPAD_BUTTON_DPAD_UP,
        SDL_GAMEPAD_BUTTON_DPAD_DOWN
    };
    memcpy(input->controller.buttons, buttons, sizeof(buttons));

    input->controller.left_stick = zeros2();
    input->controller.right_stick = zeros2();
    for (int i = 0; i < 12; i++) {
        input->controller.buttons_down[i] = false;
        input->controller.buttons_pressed[i] = false;
        input->controller.buttons_released[i] = false;
    }

    scene->components->controller[entity] = input;
    return input;
}


void ControllerComponent_remove(Entity entity) {
    ControllerComponent* controller = scene->components->controller[entity];
    if (controller) {
        free(controller);
        scene->components->controller[entity] = NULL;
    }
}
