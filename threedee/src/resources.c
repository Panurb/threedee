#include "resources.h"


void load_resources() {
    LOG_INFO("Loading resources");

    resources.textures_size = list_files_alphabetically("data/textures/*.png", resources.texture_names);
    for (int i = 0; i < resources.textures_size; i++) {
        String path;
        snprintf(path, sizeof(path), "%s%s%s", "data/textures/", resources.texture_names[i], ".png");
        resources.textures[i] = SDL_GPU_LoadTexture(app.gpu_device, path);
        if (!resources.textures[i]) {
            LOG_ERROR("Failed to load texture: %s", path);
        }
    }

    resources.sounds_size = list_files_alphabetically("data/sfx/*.wav", resources.sound_names);
    for (int i = 0; i < resources.sounds_size; i++) {
        String path;
        snprintf(path, sizeof(path), "%s%s%s", "data/sfx/", resources.sound_names[i], ".wav");
        resources.sounds[i] = Mix_LoadWAV(path);
        if (!resources.sounds[i]) {
            LOG_ERROR("Failed to load sound: %s", path);
        }
    }

    LOG_INFO("Resources loaded");
}
