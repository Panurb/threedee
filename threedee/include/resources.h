#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "settings.h"
#include "util.h"


#define MAX_TEXTURES 128
#define MAX_MATERIALS 128
#define MAX_SOUNDS 128
#define MAX_MESHES 128
#define MAX_FONTS 16
#define MAX_PARTICLE_TYPES 32
#define MAX_PHASES 10


typedef struct {
    float specular;
    float diffuse;
    float ambient;
    float shininess;
} Material;


typedef struct Mesh {
    String name;
    SDL_GPUBuffer* vertex_buffer;
    int num_vertices;
    SDL_GPUBuffer* index_buffer;
    int num_indices;
    SDL_GPUTexture* texture;
} Mesh;


typedef struct ParticlePhase {
    Color color;
    float size;
    float normalized_time;
} ParticlePhase;


typedef struct ParticleType {
    float lifetime;
    ParticlePhase phases[MAX_PHASES];
    int num_phases;
    float gravity_scale;
    int texture_index;
    bool emissive;
    float stretch;
} ParticleType;


typedef struct {
    String texture_names[MAX_TEXTURES];
    int textures_size;
    SDL_GPUTexture* texture_array;
    SDL_GPUTexture* normal_map_array;

    String emissive_map_names[MAX_TEXTURES];
    int emissive_maps_size;
    SDL_GPUTexture* emissive_map_array;

    String material_names[MAX_TEXTURES];
    int materials_size;
    Material materials[MAX_TEXTURES];
    SDL_GPUBuffer* materials_buffer;

    String particle_names[MAX_TEXTURES];
    int particles_size;
    SDL_GPUTexture* particle_array;

    String particle_type_names[MAX_PARTICLE_TYPES];
    int particle_types_size;
    ParticleType particle_types[MAX_PARTICLE_TYPES];

    String font_names[MAX_FONTS];
    TTF_Font* fonts[MAX_FONTS];
    int fonts_size;

    String sound_names[MAX_SOUNDS];
    int sounds_size;
    Mix_Chunk* sounds[100];

    Mix_Music* music[10];

    Mesh meshes[MAX_MESHES];
    String mesh_names[MAX_MESHES];
    int meshes_size;
} Resources;


Resources resources;


void load_resources();
