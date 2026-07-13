#include "components/trigger.h"

#include "scene.h"


TriggerComponent* TriggerComponent_add(Entity entity, TriggerParameters params) {
    TriggerComponent* trigger = malloc(sizeof(TriggerComponent));
    trigger->type = params.type;
    trigger->entities = ArrayList_create(sizeof(Entity));
    trigger->distance = params.distance ? params.distance : 5.0f;
    trigger->roi = params.roi ? params.roi : 1.0f;
    trigger->on_enter = params.on_enter;
    trigger->on_exit = params.on_exit;
    trigger->on_stay = params.on_stay;
    trigger->trigger_group = params.trigger_group ? params.trigger_group : GROUP_ALL;
    trigger->target_entity = params.target_entity ? params.target_entity : NULL_ENTITY;
    trigger->level = params.level;
    trigger->timer = 0.0f;
    trigger->max_time = params.max_time;

    scene->components->trigger[entity] = trigger;

    return trigger;
}


void TriggerComponent_remove(Entity entity) {
    TriggerComponent* trigger = get_component(entity, COMPONENT_TRIGGER);
    if (trigger) {
        free(trigger);
        scene->components->trigger[entity] = NULL;
    }
}
