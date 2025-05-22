#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL_mixer.h>

#include "component.h"
#include "collider.h"
#include "image.h"
#include "sound.h"


#define MAX_TEXTURES 128
#define MAX_SOUNDS 128


typedef enum {
    STATE_MENU,
    STATE_START,
    STATE_END,
    STATE_RESET,
    STATE_GAME,
    STATE_PAUSE,
    STATE_SAVE,
    STATE_LOAD,
    STATE_APPLY,
    STATE_CREATE,
    STATE_LOAD_EDITOR,
    STATE_EDITOR,
    STATE_QUIT,
    STATE_GAME_OVER,
    STATE_INTRO
} GameState;


typedef enum {
    MODE_SURVIVAL,
    MODE_CAMPAIGN,
    MODE_TUTORIAL
} GameMode;


extern ButtonText GAME_MODES[];
extern ButtonText WEATHERS[];


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
    SDL_Texture* blood_particle;
} Resources;


typedef enum {
    WEATHER_NONE,
    WEATHER_RAIN,
    WEATHER_SNOW
} Weather;


typedef struct {
    ComponentData* components;
    ColliderGrid* grid;
    float ambient_light;
    Weather weather;
    Vector2 wind;
    float wind_speed;
    int seed;
    int camera;
    int menu_camera;
    ButtonText map_name;
    GameMode game_mode;
    bool testing;
    Vector2 start_position;
    int music;
} GameData;


// Globals
extern GameState game_state;
extern GameData* game_data;

extern Resources resources;


void change_state_win();

void load_resources();

void create_game();

void resize_game();

void init_game();

void start_game(Filename map_name, bool load_save);

void end_game();

void reset_game();

void update_coordinates();

void update_game(float time_step);

void update_game_mode(float time_step);

void draw_game();

void draw_debug(int debug_level);

void update_game_over(float time_step);

void draw_game_over();

void draw_game_mode();

int create_tutorial(Vector2 position);

int create_level_end(Vector2 position, float angle, float width, float height);

void draw_tutorials();
