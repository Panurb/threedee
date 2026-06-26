#pragma once


Entity create_player(Vector3 position);

Entity get_current_item(Entity player);

bool in_player_view(Entity player, Entity entity, float distance, float roi);

void update_players(float time_step);

void input_players(void);

Entity get_player_camera(Entity player);

void player_look(Entity trigger, Entity entity);
