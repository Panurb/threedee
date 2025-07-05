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
    scene->components->player[entity] = component;
    return component;
}


void PlayerComponent_remove(Entity entity) {
    PlayerComponent* component = get_component(entity, COMPONENT_PLAYER);
    if (component) {
        free(component);
        scene->components->player[entity] = NULL;
    }
}
