#pragma once


typedef struct SoundEvent {
    bool loop;
    int channel;
    float volume;
    float pitch;
    String filename;
} SoundEvent;


typedef struct SoundComponent {
    int size;
    SoundEvent* events[4];
    String hit_sound;
    String loop_sound;
} SoundComponent;


typedef struct SoundParameters {
    String hit_sound;
    String loop_sound;
} SoundParameters;


SoundComponent* SoundComponent_add(Entity entity, SoundParameters params);

void SoundComponent_remove(Entity entity);
