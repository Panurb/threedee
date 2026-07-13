#include <stdio.h>

#include "component.h"
#include "scene.h"


static float scare_timer = 10.0f;


bool is_trigger(Entity entity) {
    TriggerComponent* trigger = get_component(entity, COMPONENT_TRIGGER);
    return trigger && trigger->type == TRIGGER_MANUAL && trigger->level <= scene->scare_level;
}


void update_game(float time_step) {
    if (scare_timer > 0.0f) {
        scare_timer -= time_step;
    } else {
        LOG_INFO("Checking for triggers in radius");
        scare_timer = 0.0f;

        List* entities = get_entities_in_radius(
            get_position(scene->player),
            10.0f,
            is_trigger
        );

        if (entities->size > 0) {
            int i = randi(0, entities->size - 1);
            Entity entity = List_get(entities, i)->value;
            TriggerComponent* trigger = get_component(entity, COMPONENT_TRIGGER);
            if (trigger->on_enter) {
                trigger->on_enter(entity, scene->player);
                destroy_entity_recursive(entity);
                scare_timer = 10.0f;
                scene->scare_level++;
            }
        } else {
            scare_timer = 10.0f;
        }

        List_delete(entities);
    }
}
