#define _USE_MATH_DEFINES

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "sound.h"
#include "util.h"
#include "component.h"
#include "settings.h"
#include "resources.h"
#include "scene.h"


void load_sounds() {
    LOG_INFO("Loading sounds");

    resources.sounds_size = list_files_alphabetically("data/sfx/*.wav", resources.sound_names); 

    for (int i = 0; i < resources.sounds_size; i++) {
        String path;
        snprintf(path, sizeof(path), "%s%s%s", "data/sfx/", resources.sound_names[i], ".wav");

        resources.sounds[i] = Mix_LoadWAV(path);
    }
}


int sound_index(Filename filename) {
    return binary_search_filename(filename, resources.sound_names, resources.sounds_size);
}


void add_sound(int entity, Filename filename, float volume, float pitch) {
    SoundComponent* scomp = SoundComponent_get(entity);
    for (int i = 0; i < scomp->size; i++) {
        if (!scomp->events[i]) {
            SoundEvent* event = malloc(sizeof(SoundEvent));
            strcpy(event->filename, filename);
            event->volume = volume;
            event->pitch = pitch;
            event->loop = false;
            event->channel = -1;
            scomp->events[i] = event;
            break;
        }
    }
}


void loop_sound(int entity, Filename filename, float volume, float pitch) {
    SoundComponent* scomp = SoundComponent_get(entity);
    for (int i = 0; i < scomp->size; i++) {
        if (!scomp->events[i]) {
            SoundEvent* event = malloc(sizeof(SoundEvent));
            strcpy(event->filename, filename);
            event->volume = volume;
            event->pitch = pitch;
            event->loop = true;
            event->channel = -1;
            scomp->events[i] = event;
            break;
        }
    }
}


void stop_loop(int entity) {
    SoundComponent* scomp = SoundComponent_get(entity);
    for (int i = 0; i < scomp->size; i++) {
        SoundEvent* event = scomp->events[i];
        if (event) {
            if (event->loop) {
                event->loop = false;
            }
        }
    }
}


void play_sounds(int camera) {
    for (int i = 0; i < scene->components->entities; i++) {
        SoundComponent* scomp = SoundComponent_get(i);
        if (!scomp) continue;

        float dist = norm2(diff(get_xy(i), get_xy(camera)));

        if (scomp->loop_sound[0] != '\0') {
            if (!scomp->events[0]) {
                loop_sound(i, scomp->loop_sound, 0.5f, 1.0f);
            }
        }

        for (int j = 0; j < scomp->size; j++) {
            SoundEvent* event = scomp->events[j];
            if (!event) continue;

            int chan = event->channel;

            float vol = event->volume;

            if (chan != -1) {
                if (!event->loop) {
                    event->volume *= 0.95;
                    if (event->volume < 0.01) {
                        Mix_HaltChannel(chan);
                        free(event);
                        scomp->events[j] = NULL;
                    }
                }
            } else {
                int loops = event->loop ? -1 : 0;
                chan = Mix_PlayChannel(chan, resources.sounds[sound_index(event->filename)], loops);

                if (event->loop) {
                    event->channel = chan;
                } else {
                    free(event);
                    scomp->events[j] = NULL;
                }
            }

            Mix_Volume(chan, vol * MIX_MAX_VOLUME * game_settings.volume / 100.0f);
            // TODO: pitch
        }
    }
}


void clear_sounds(int entity) {
    SoundComponent* scomp = SoundComponent_get(entity);
    for (int i = 0; i < scomp->size; i++) {
        SoundEvent* event = scomp->events[i];
        if (event) {
            Mix_HaltChannel(event->channel);
            free(event);
            scomp->events[i] = NULL;
        }
    }
}


void clear_all_sounds() {
    Mix_HaltChannel(-1);
}
