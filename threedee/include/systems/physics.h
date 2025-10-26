#pragma once


void apply_impulse(Entity entity, Vector3 point, Vector3 impulse);

void apply_force(Entity entity, Vector3 point, Vector3 force);

void init_physics(void);

void update_physics(float time_step);
