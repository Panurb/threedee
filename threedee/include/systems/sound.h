#pragma once

#include "../component.h"

// Footstep
// https://freesound.org/people/Ali_6868/sounds/384889/


void add_sound(int entity, String filename, float volume, float pitch);

void loop_sound(int entity, String filename, float volume, float pitch);

void stop_loop(int entity);

void play_sounds(int camera);

void clear_sounds(int entity);

void clear_all_sounds();
