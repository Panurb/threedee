#include <stdio.h>
#include <time.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "app.h"
#include "game.h"

#include <math.h>
#include <stdlib.h>

#include "camera.h"
#include "component.h"
#include "util.h"
#include "image.h"
#include "grid.h"
#include "navigation.h"
#include "sound.h"
#include "settings.h"


Scene* game_data;

Resources resources;


void load_resources() {
    load_textures();
    LOG_INFO("Loaded %d textures", resources.textures_size);
    resources.fonts[0] = NULL;
    for (int size = 1; size <= 300; size++) {
        resources.fonts[size] = TTF_OpenFont("data/Helvetica.ttf", size);
        if (!resources.fonts[size]) {
            fprintf(stderr, "Error loading font: %s\n", SDL_GetError());
            exit(1);
        }
    }
    LOG_INFO("Loaded %d fonts", 300);
    load_sounds();
    resources.music[0] = Mix_LoadMUS("data/music/zsong.ogg");
    resources.music[1] = Mix_LoadMUS("data/music/zsong2.ogg");
}


void create_game() {
    game_data = malloc(sizeof(Scene));

    strcpy(game_data->map_name, "");

    game_data->components = ComponentData_create();
    ColliderGrid* grid = ColliderGrid_create();
    float ambient_light = 0.5f;
    int seed = time(NULL);
    int camera = create_camera();
    int menu_camera = create_menu_camera();

    game_data->grid = grid;
    game_data->camera = camera;
    game_data->menu_camera = menu_camera;

    game_data->game_mode = MODE_SURVIVAL;

    game_data->music = 0;
}


void resize_game() {
    for (int i = 0; i < game_data->components->entities; i++) {
        CameraComponent* camera = CameraComponent_get(i);
        if (camera) {
            camera->resolution.w = game_settings.width;
            camera->resolution.h = game_settings.height;
            camera->zoom = camera->zoom_target * camera->resolution.h / 720.0;
        }
    }
}


void init_game() {
    int i = 0;
    ListNode* node;
    FOREACH(node, game_data->components->player.order) {
        if (app.player_controllers[i] == CONTROLLER_NONE) {
            destroy_entity_recursive(node->value);
        } else {
            PlayerComponent* player = PlayerComponent_get(node->value);
            player->controller.joystick = app.player_controllers[i];
            LOG_DEBUG("Player %d: %d\n", i, player->controller.joystick);
        }

        i++;
    }

    init_grid();
    init_waypoints();
}


void update_lifetimes(float time_step) {
    for (int i = 0; i < game_data->components->entities; i++) {
        CoordinateComponent* coord = CoordinateComponent_get(i);
        if (!coord) continue;

        if (coord->lifetime > 0.0f) {
            coord->lifetime = fmaxf(0.0f, coord->lifetime - time_step);

            ImageComponent* image = ImageComponent_get(i);
            if (coord->lifetime < 1.0f) {
                if (image) {
                    image->alpha = fminf(image->alpha, coord->lifetime);
                }
            }
        } else if (coord->lifetime == 0.0f) {
            LightComponent* light = LightComponent_get(i);
            if (light) {
                light->enabled = false;
            }

            ParticleComponent* particle = ParticleComponent_get(i);
            if (particle) {
                particle->loop = false;
                particle->enabled = false;
                if (particle->particles > 0) {
                    continue;
                }
            }
            if (ColliderComponent_get(i)) {
                clear_grid(i);
            }
            if (SoundComponent_get(i)) {
                clear_sounds(i);
            }
            destroy_entity_recursive(i);
        }
    }
}


void update_coordinates() {
    for (int i = 0; i < game_data->components->entities; i++) {
        CoordinateComponent* coord = CoordinateComponent_get(i);
        if (coord) {
            coord->previous.position = get_position(i);
            coord->previous.angle = get_angle(i);
            coord->previous.scale = get_scale(i);
        }
    }
}


void update_game(float time_step) {

}


void draw_game() {

}


void draw_debug(int debug_level) {

}
