#pragma once

#include <stdbool.h>

#include <SDL.h>

#include "interface.h"

#define CONTROLLER_NONE -2
#define CONTROLLER_MKB -1

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GameController* controllers[8];
    int player_controllers[4];
    bool quit;
    bool focus;
    FpsCounter* fps;
    float time_step;
    float delta;
} App;

extern App app;

void init();

void quit();

void input();

void update(float time_step);

void draw();

void play_audio();
