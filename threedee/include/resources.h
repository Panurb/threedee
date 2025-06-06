#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "util.h"


#define MAX_TEXTURES 128
#define MAX_MATERIALS 128
#define MAX_SOUNDS 128
#define MAX_MESHES 128


typedef struct {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
} Material;


typedef struct MeshData{
    SDL_GPUBuffer* vertex_buffer;
    int num_vertices;
    SDL_GPUBuffer* index_buffer;
    int num_indices;
    SDL_GPUBuffer* instance_buffer;
    int num_instances;
    int max_instances;
    SDL_GPUTransferBuffer* instance_transfer_buffer;
    SDL_GPUSampler* sampler;
} MeshData;


typedef struct {
    String texture_names[MAX_TEXTURES];
    int textures_size;
    Resolution texture_sizes[MAX_TEXTURES];
    SDL_GPUTexture* texture_atlas;
    Rect texture_rects[MAX_TEXTURES];

    String material_names[MAX_TEXTURES];
    int materials_size;
    Material materials[MAX_TEXTURES];

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
