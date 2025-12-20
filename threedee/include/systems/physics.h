#pragma once


#define SLEEP_THRESHOLD_LINEAR 0.1f
#define SLEEP_THRESHOLD_ANGULAR 0.05f


void apply_impulse(Entity entity, Vector3 point, Vector3 impulse);

void apply_force(Entity entity, Vector3 point, Vector3 force);

void init_physics(void);

void update_physics(float time_step);
