#define _USE_MATH_DEFINES

#include "component.h"
#include "game.h"


void update_players(float time_step) {
    for (int i = 0; i < game_data->components->entities; i++) {
        PlayerComponent* player = PlayerComponent_get(i);
        if (!player) continue;

        switch (player->state) {

        }
    }
}


void player_die(int entity) {

}
