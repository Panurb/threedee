#define _USE_MATH_DEFINES

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "../../include/systems/sound.h"
#include "util.h"
#include "component.h"
#include "settings.h"
#include "resources.h"
#include "scene.h"


int sound_index(String filename) {
    return binary_search_filename(filename, resources.sound_names, resources.sounds_size);
}


void add_sound(Entity entity, String filename, float volume, float pitch) {
    SoundComponent* scomp = get_component(entity, COMPONENT_SOUND);
    if (scomp->cooldown_timer > 0.0f) {
        return;
    }

    for (int i = 0; i < scomp->size; i++) {
        if (!scomp->events[i]) {
            SoundEvent* event = malloc(sizeof(SoundEvent));
            strcpy(event->filename, filename);
            event->volume = volume * scomp->volume;
            event->pitch = pitch;
            event->loop = false;
            event->channel = -1;
            scomp->events[i] = event;
            scomp->cooldown_timer = scomp->cooldown;
            break;
        }
    }
}


void loop_sound(int entity, String filename, float volume, float pitch) {
    SoundComponent* scomp = get_component(entity, COMPONENT_SOUND);
    if (!scomp) {
        LOG_WARNING("Entity %d does not have a sound component, cannot loop sound %s", entity, filename);
        return;
    }

    int free_slot = -1;
    for (int i = 0; i < scomp->size; i++) {
        SoundEvent* event = scomp->events[i];
        if (event) {
            if (strcmp(event->filename, filename) == 0) {
                event->volume = volume;
                event->pitch = pitch;
                return;
            }
        } else if (free_slot == -1) {
            free_slot = i;
        }
    }

    if (free_slot != -1) {
        SoundEvent* event = malloc(sizeof(SoundEvent));
        strcpy(event->filename, filename);
        event->volume = volume;
        event->pitch = pitch;
        event->loop = true;
        event->channel = -1;
        scomp->events[free_slot] = event;
    }
}


void stop_loop(int entity) {
    SoundComponent* scomp = get_component(entity, COMPONENT_SOUND);
    for (int i = 0; i < scomp->size; i++) {
        SoundEvent* event = scomp->events[i];
        if (event) {
            if (event->loop) {
                event->loop = false;
            }
        }
    }
}


void set_panning_from_angle(int channel, float angle) {
    // There seems to be a bug in SDL_mixer so have to do this by hand

    angle = fmodf(angle + 360.0f, 360.0f);
    float pan = sinf(to_radians(angle));

    Uint8 left = 255 * (1.0f - pan) / 2.0f;
    Uint8 right = 255 * (1.0f + pan) / 2.0f;

    Mix_SetPanning(channel, left, right);
}


void update_sounds(float time_step) {
    for (int i = 0; i < scene->components->entities; i++) {
        SoundComponent* scomp = get_component(i, COMPONENT_SOUND);
        if (!scomp) continue;

        scomp->cooldown_timer = fmaxf(0.0f, scomp->cooldown_timer - time_step);
    }
}


void play_sounds(int camera) {
    for (int i = 0; i < scene->components->entities; i++) {
        SoundComponent* scomp = get_component(i, COMPONENT_SOUND);
        if (!scomp) continue;

        Vector3 position = get_position(i);
        float dist = norm3(sub3(position, get_position(camera)));
        float max_dist = 100.0f;

        Matrix4 camera_transform = get_transform(camera);

        Vector4 position_rel = map4(inverse_transform(camera_transform), vec4(position.x, position.y, position.z, 1.0f));

        float angle = to_degrees(atan2f(position_rel.x, position_rel.z));

        if (scomp->loop_sound[0] != '\0') {
            if (!scomp->events[0]) {
                loop_sound(i, scomp->loop_sound, scomp->volume, 1.0f);
            }
        }

        for (int j = 0; j < scomp->size; j++) {
            SoundEvent* event = scomp->events[j];
            if (!event) continue;

            int chan = event->channel;

            set_panning_from_angle(chan, angle);
            Mix_SetDistance(chan, (int) (fminf(dist / max_dist, 1.0f) * 255.0f));
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

            Mix_Volume(chan, event->volume * MIX_MAX_VOLUME * game_settings.volume / 100.0f);
            // TODO: pitch
        }
    }
}


void clear_sounds(int entity) {
    SoundComponent* scomp = get_component(entity, COMPONENT_SOUND);
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
