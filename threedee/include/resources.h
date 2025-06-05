#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "util.h"
#include "render.h"


#define MAX_TEXTURES 128
#define MAX_SOUNDS 128
#define MAX_MESHES 128

typedef struct {
    String texture_names[MAX_TEXTURES];
    int textures_size;
    Resolution texture_sizes[MAX_TEXTURES];
    SDL_GPUTexture* texture_atlas;
    Rect texture_rects[MAX_TEXTURES];

    TTF_Font* fonts[301];

    String sound_names[MAX_SOUNDS];
    int sounds_size;
    Mix_Chunk* sounds[100];

    Mix_Music* music[10];

    MeshData meshes[MAX_MESHES];
    String mesh_names[MAX_MESHES];
    int meshes_size;
} Resources;


Resources resources;


void load_resources();
