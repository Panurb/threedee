#include <stdlib.h>

#include "components/player.h"
#include "component.h"
#include "util.h"
#include "scene.h"


PlayerComponent* PlayerComponent_add(Entity entity) {
    PlayerComponent* component = malloc(sizeof(PlayerComponent));
    component->yaw = 0.0f;
    component->pitch = 0.0f;
    component->grabbed_entity = NULL_ENTITY;
    component->examine_yaw = 0.0f;
    component->examining = false;
    component->inventory = ArrayList_create(sizeof(Entity));
    component->selected_item = 0;
    component->footstep_timer = 0.0f;
    component->footstep_interval = 1.0f;
    component->foot = FOOT_BOTH;
    component->head_height = 1.0f;
    component->view_bobbing = 0.03f;
    component->sprinting = false;
    component->walk_speed = 3.0f;
    component->sprint_speed = 5.0f;
    component->max_sprint = 2.0f;
    component->sprint_timer = component->max_sprint;
    scene->components->player[entity] = component;
    return component;
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
