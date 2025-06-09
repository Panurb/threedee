#pragma once

#include <SDL3_image/SDL_image.h>

#include "component.h"


typedef enum {
    SHADER_NONE,
    SHADER_OUTLINE
} CameraShader;

int create_camera();

int create_menu_camera();

Vector2 camera_size(int camera);

Vector3 look_direction(Entity camera);

Vector3 world_to_screen(int camera, Vector3 a);

Vector3 screen_to_world(int camera, Vector3 a);

void draw_triangle_fan(int camera, SDL_Vertex* vertices, int verts_size);

void draw_line(int camera, Vector2 start, Vector2 end, float width, Color color);

void draw_circle(int camera, Vector2 position, float radius, Color color);

void draw_ellipse_two_color(int camera, Vector2 position, float major, float minor, float angle, 
    Color color, Color color_center);

void draw_circle_outline(int camera, Vector2 position, float radius, float line_width, Color color);

void draw_ellipse(int camera, Vector2 position, float major, float minor, float angle, Color color);

void draw_rectangle(int camera, Vector2 position, float width, float height, float angle, Color color);

void draw_rectangle_outline(int camera, Vector2 position, float width, float height, 
    float angle, float line_width, Color color);

void draw_slice(int camera, Vector2 position, float min_range, float max_range, 
    float angle, float spread, Color color);

void draw_slice_outline(int camera, Vector2 position, float min_range, float max_range, 
    float angle, float spread);

void draw_sprite(int camera, int texture_index, float width, float height, int offset, Vector2 position, float angle, 
    Vector2 scale, float alpha);

void draw_tiles(int camera, int texture_index, float width, float height, Vector2 offset, Vector2 position, 
        float angle, Vector2 scale, float alpha);

void draw_sprite_outline(int camera, int texture_index, float width, float height, int offset, Vector2 position, 
        float angle, Vector2 scale);

void draw_text(int camera, Vector2 position, char string[100], int size, Color color);

void draw_spline(int camera, int texture_index, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float width, bool flip, Color color);

void update_camera(Entity camera, float time_step);

bool on_screen(int camera, Vector2 position, float width, float height);
