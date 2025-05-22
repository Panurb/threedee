#pragma once

#include <SDL3/SDL.h>

#include "component.h"


#define BUTTON_WIDTH 8.0f
#define BUTTON_HEIGHT 2.0f
#define MARGIN 0.3f
#define BORDER_WIDTH 0.2f
#define SCROLLBAR_WIDTH 1.0f
#define COLOR_CONTAINER get_color(0.15f, 0.15f, 0.15f, 1.0f)
#define COLOR_SHADOW get_color(0.2f, 0.2f, 0.2f, 1.0f)
#define COLOR_BUTTON get_color(0.3f, 0.3f, 0.3f, 1.0f)
#define COLOR_SELECTED get_color(0.4f, 0.4f, 0.4f, 1.0f)
#define COLOR_BORDER get_color(0.1f, 0.1f, 0.1f, 1.0f)
#define COLOR_TEXT COLOR_WHITE

void bring_to_top(int entity);

int create_window(Vector2 position, ButtonText text, int width, OnClick on_close);

int create_label(ButtonText text, Vector2 position);

int create_button(ButtonText text, Vector2 position, OnClick on_click);

int create_button_small(ButtonText text, Vector2 position, OnClick on_click);

int create_container(Vector2 position, int width, int height);

void add_widget_to_container(int container, int entity);

int add_button_to_container(int container, ButtonText string, OnClick on_click);

void increment_value(int entity, int direction);

void add_row_to_container(int container, int left, int right);

void add_files_to_container(int container, Filename path, OnClick on_click);

int create_dropdown(Vector2 position, ButtonText* strings, int size);

void set_slider(int entity, Vector2 mouse_position);

int create_slider(Vector2 position, int min_value, int max_value, int value,
    OnChange on_change);

void set_scrollbar(int entity, Vector2 mouse_position);

int create_scrollbar(Vector2 position, int height, int max_value, OnChange on_change);

void add_scrollbar_to_container(int container);

int create_textbox(Vector2 position, int width);

int create_checkbox(Vector2 position, bool value, OnChange on_change);

void update_widgets(int camera);

void draw_widgets(int camera);

bool input_widgets(int camera, SDL_Event event);

void destroy_widgets();
