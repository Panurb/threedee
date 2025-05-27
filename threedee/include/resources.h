#pragma once

#include <SDL3_ttf/SDL_ttf.h>


#define MAX_TEXTURES 100
#define MAX_SOUNDS 100

typedef struct {
    String texture_names[MAX_TEXTURES];
    int textures_size;
    SDL_Texture* textures[MAX_TEXTURES];
    Resolution texture_sizes[MAX_TEXTURES];
    TTF_Font* fonts[301];
    String sound_names[MAX_SOUNDS];
    int sounds_size;
    Mix_Chunk* sounds[100];
    Mix_Music* music[10];
} Resources;


Resources resources;
