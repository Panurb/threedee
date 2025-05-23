#pragma once

#include <stdbool.h>

#include <SDL3/SDL.h>

#include "interface.h"

#define CONTROLLER_NONE -2
#define CONTROLLER_MKB -1


typedef enum {
    STATE_MENU,
    STATE_START,
    STATE_END,
    STATE_RESET,
    STATE_GAME,
    STATE_PAUSE,
    STATE_SAVE,
    STATE_LOAD,
    STATE_APPLY,
    STATE_CREATE,
    STATE_LOAD_EDITOR,
    STATE_EDITOR,
    STATE_QUIT,
    STATE_GAME_OVER,
    STATE_INTRO
} AppState;


typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;  // TODO: remove
    SDL_GPUDevice* gpu_device;
    SDL_Gamepad* controllers[8];
    int player_controllers[4];
    bool quit;
    bool focus;
    FpsCounter* fps;
    float time_step;
    float delta;
    AppState state;
    const char* base_path;
} App;


extern App app;

void init();

void quit();

void input();

void update(float time_step);

void draw();

void play_audio();
