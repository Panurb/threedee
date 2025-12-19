#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "components/sound.h"
#include "scene.h"


SoundComponent* SoundComponent_add(Entity entity, SoundParameters params) {
    SoundComponent* sound = malloc(sizeof(SoundComponent));
    sound->size = 4;
    for (int i = 0; i < sound->size; i++) {
        sound->events[i] = NULL;
    }
    strcpy(sound->hit_sound, params.hit_sound);
    strcpy(sound->loop_sound, params.loop_sound);
    sound->cooldown = 0.2f;
    sound->cooldown_timer = 1.0f;
    scene->components->sound[entity] = sound;
    return sound;
}


void SoundComponent_remove(Entity entity) {
    SoundComponent* sound = get_component(entity, COMPONENT_SOUND);
    if (sound) {
        free(sound);
        scene->components->sound[entity] = NULL;
    }
}
