#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "util.h"
#include "component.h"
#include "camera.h"
#include "collider.h"
#include "raycast.h"
#include "util.h"
#include "weapon.h"
#include "vehicle.h"
#include "item.h"
#include "image.h"
#include "input.h"
#include "particle.h"
#include "enemy.h"
#include "health.h"
#include "door.h"



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
