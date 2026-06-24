#include <stdlib.h>

#include "components/player.h"
#include "component.h"
#include "util.h"
#include "scene.h"


PlayerComponent* PlayerComponent_add(Entity entity) {
    PlayerComponent* player = malloc(sizeof(PlayerComponent));
    player->yaw = 0.0f;
    player->pitch = 0.0f;
    player->grabbed_entity = NULL_ENTITY;
    player->examine_yaw = 0.0f;
    player->examining = false;
    player->inventory = ArrayList_create(sizeof(Entity));
    player->selected_item = 0;
    player->footstep_timer = 0.0f;
    player->footstep_interval = 1.0f;
    player->foot = FOOT_BOTH;
    player->head_height = 1.0f;
    player->view_bobbing = 0.03f;
    player->sprinting = false;
    player->walk_speed = 3.0f;
    player->sprint_speed = 5.0f;
    player->max_sprint = 2.0f;
    player->sprint_timer = player->max_sprint;
    player->look_target = NULL_ENTITY;
    player->look_timer = 0.0f;
    scene->components->player[entity] = player;
    return player;
}


void PlayerComponent_remove(Entity entity) {
    PlayerComponent* component = get_component(entity, COMPONENT_PLAYER);
    if (component) {
        ArrayList_for_each(component->inventory, destroy_entity);
        ArrayList_destroy(component->inventory);
        free(component);
        scene->components->player[entity] = NULL;
    }
}
