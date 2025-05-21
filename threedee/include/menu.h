#pragma once

#include "util.h"
#include "component.h"
#include "game.h"


void create_menu();

void destroy_menu();

void create_pause_menu();

void update_menu();

void input_menu(int camera, SDL_Event event);

void draw_menu();

void create_game_over_menu();

void create_win_menu();
