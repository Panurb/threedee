#pragma once

#include <SDL.h>

#include "component.h"
#include "camera.h"
#include "perlin.h"


#define PIXELS_PER_UNIT 128


float image_width(int entity);
float image_height(int entity);

int create_decal(Vector2f pos, Filename filename, float lifetime);

void load_textures();

int get_texture_index(Filename filename);

void set_texture(ImageComponent* image);

void draw_ground(int camera);

void draw_images(int camera);

void draw_roofs(int camera);

void change_texture(int entity, Filename filename, float width, float height);

void change_layer(int entity, Layer layer);

// void create_noise(sfUint8* pixels, int width, int height, Vector2f origin, Color color, float sharpness, Permutation p);

bool point_inside_image(int entity, Vector2f point);
