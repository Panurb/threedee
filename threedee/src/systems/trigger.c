#include "systems\trigger.h"
#include "util.h"
#include "scene.h"
#include "systems/player.h"


void update_triggers(float time_step) {
    for (Entity i = 0; i < scene->components->entities; i++) {
        TriggerComponent* trigger = get_component(i, COMPONENT_TRIGGER);
        if (!trigger) continue;

        Vector3 position = get_position(i);

        for (int j = 0; j < trigger->entities->size; j++) {
            Entity other = *(Entity*)ArrayList_get(trigger->entities, j);
            if (trigger->on_stay) {
                trigger->on_stay(i, other, time_step);
            }
        }

        if (trigger->type == TRIGGER_LOOK) {
            for (int j = 0; j < scene->components->entities; j++) {
                PlayerComponent* player = get_component(j, COMPONENT_PLAYER);
                if (!player) continue;

                int k = ArrayList_find(trigger->entities, &j);
                if (in_player_view(j, i, trigger->distance, trigger->roi)) {
                    if (k == -1) {
                        ArrayList_add(trigger->entities, &j);
                        if (trigger->on_enter) {
                            trigger->on_enter(i, j);
                        }
                    }
                } else {
                    if (k != -1) {
                        ArrayList_remove(trigger->entities, k);
                        if (trigger->on_exit) {
                            trigger->on_exit(i, j);
                        }
                    }
                }
            }
        }
    }
}
