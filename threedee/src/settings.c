#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <SDL3/SDL.h>

#include "settings.h"
#include "../include/systems/input.h"


Settings game_settings = {
    .width = 1280,
    .height = 720,
    .antialiasing = 8,
    .fullscreen = false,
    .vsync = false,
    .max_fps = 0,
    .volume = 100,
    .music = 100,
    .particles = QUALITY_LOW,
    .debug = false,
    .keybinds = {
        { DEVICE_KEYBOARD, SDL_SCANCODE_W },
        { DEVICE_KEYBOARD, SDL_SCANCODE_A },
        { DEVICE_KEYBOARD, SDL_SCANCODE_S },
        { DEVICE_KEYBOARD, SDL_SCANCODE_D },
        { DEVICE_MOUSE, SDL_BUTTON_LEFT },
        { DEVICE_KEYBOARD, SDL_SCANCODE_LALT },
        { DEVICE_MOUSE, SDL_BUTTON_RIGHT },
        { DEVICE_KEYBOARD, SDL_SCANCODE_R },
        { DEVICE_KEYBOARD, SDL_SCANCODE_F },
        { DEVICE_KEYBOARD, SDL_SCANCODE_E },
        { DEVICE_KEYBOARD, SDL_SCANCODE_LSHIFT }
    },
    .mouse_sensitivity = 1.0f,
    .fov = 70.0f
};


KeyValue parse_line(Line string, char* delim) {
    char* token = strtok(string, delim);
    char* key = token;
    token = strtok(NULL, delim);
    char* value = token;

    if (value[strlen(value) - 1] == '\n') {
        value[strlen(value) - 1] = '\0';
    }

    return (KeyValue) { key, value };
}


void load_settings() {
    FILE* file = fopen("config.cfg", "r");

    if (!file) {
        return;
    }

    Line buffer;
    while (fgets(buffer, 255, file)) {
        KeyValue line = parse_line(buffer, "=");

        if (strcmp(line.key, "WIDTH") == 0) {
            game_settings.width = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "HEIGHT") == 0) {
            game_settings.height = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "ANTIALIASING") == 0) {
            game_settings.antialiasing = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "FULLSCREEN") == 0) {
            game_settings.fullscreen = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "VSYNC") == 0) {
            game_settings.vsync = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "MAX_FPS") == 0) {
            game_settings.max_fps = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "VOLUME") == 0) {
            game_settings.volume = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "MUSIC") == 0) {
            game_settings.music = strtol(line.value, NULL, 10);
        } else if (strcmp(line.key, "DEBUG") == 0) {
            game_settings.debug = strtol(line.value, NULL, 10);
        } else {
            for (int i = 0; i < ACTIONS_SIZE; i++) {
                if (strcmp(line.key, ACTIONS[i]) == 0) {
                    game_settings.keybinds[i] = string_to_keybind(line.value);
                }
            }
        }
    }

    fclose(file);
}


void save_settings() {
    FILE* file = fopen("config.cfg", "w");

    fprintf(file, "WIDTH=%i\n", game_settings.width);
    fprintf(file, "HEIGHT=%i\n", game_settings.height);
    fprintf(file, "ANTIALIASING=%i\n", game_settings.antialiasing);
    fprintf(file, "FULLSCREEN=%i\n", game_settings.fullscreen);
    fprintf(file, "VSYNC=%i\n", game_settings.vsync);
    fprintf(file, "MAX_FPS=%i\n", game_settings.max_fps);
    fprintf(file, "VOLUME=%i\n", game_settings.volume);
    fprintf(file, "MUSIC=%i\n", game_settings.music);
    for (int i = 0; i < ACTIONS_SIZE; i++) {
        fprintf(file, "%s=%s\n", ACTIONS[i], keybind_to_string(game_settings.keybinds[i]));
    }

    fclose(file);
}
