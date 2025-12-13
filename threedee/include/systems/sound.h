#pragma once

#include "../component.h"

// Footstep
// https://freesound.org/people/Ali_6868/sounds/384889/


void add_sound(Entity entity, String filename, float volume, float pitch);

void loop_sound(Entity entity, String filename, float volume, float pitch);

void stop_loop(Entity entity);

void update_sounds(float time_step);

void play_sounds(Entity camera);

void clear_sounds(Entity entity);

void clear_all_sounds();
