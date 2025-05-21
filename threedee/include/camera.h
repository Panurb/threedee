#pragma once

#include <SDL_image.h>

#include "component.h"


typedef enum {
    SHADER_NONE,
    SHADER_OUTLINE
} CameraShader;

int create_camera();

int create_menu_camera();

Vector2f camera_size(int camera);

Vector2f world_to_screen(int camera, Vector2f a);

Vector2f screen_to_world(int camera, Vector2f a);

void draw_triangle_fan(int camera, SDL_Vertex* vertices, int verts_size);

void draw_line(int camera, Vector2f start, Vector2f end, float width, Color color);

void draw_circle(int camera, Vector2f position, float radius, Color color);

void draw_ellipse_two_color(int camera, Vector2f position, float major, float minor, float angle, 
    Color color, Color color_center);

void draw_circle_outline(int camera, Vector2f position, float radius, float line_width, Color color);

void draw_ellipse(int camera, Vector2f position, float major, float minor, float angle, Color color);

void draw_rectangle(int camera, Vector2f position, float width, float height, float angle, Color color);

void draw_rectangle_outline(int camera, Vector2f position, float width, float height, 
    float angle, float line_width, Color color);

void draw_slice(int camera, Vector2f position, float min_range, float max_range, 
    float angle, float spread, Color color);

void draw_slice_outline(int camera, Vector2f position, float min_range, float max_range, 
    float angle, float spread);

void draw_sprite(int camera, int texture_index, float width, float height, int offset, Vector2f position, float angle, 
    Vector2f scale, float alpha);

void draw_tiles(int camera, int texture_index, float width, float height, Vector2f offset, Vector2f position, 
        float angle, Vector2f scale, float alpha);

void draw_sprite_outline(int camera, int texture_index, float width, float height, int offset, Vector2f position, 
        float angle, Vector2f scale);

void draw_text(int camera, Vector2f position, char string[100], int size, Color color);

void draw_spline(int camera, int texture_index, Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3, float width, bool flip, Color color);

void update_camera(int camera, float time_step, bool follow_players);

bool on_screen(int camera, Vector2f position, float width, float height);

void shake_camera(float speed);

void draw_overlay(int camera, float alpha);
