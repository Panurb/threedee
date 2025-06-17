#pragma once


#include "systems/input.h"

#include "util.h"
#include "linalg.h"


typedef struct {
    int joystick;
    int buttons[12];
    bool buttons_down[12];
    bool buttons_pressed[12];
    bool buttons_released[12];
    int axes[8];
    Vector2 left_stick;
    Vector2 right_stick;
    Vector2 dpad;
    float left_trigger;
    float right_trigger;
} Controller;


typedef struct {
    Controller controller;
    void (*on_button_down)(ControllerButton button);
} ControllerComponent;


ControllerComponent* ControllerComponent_add(Entity entity, int joystick);

void ControllerComponent_remove(Entity entity);
