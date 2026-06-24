#pragma once

#include "util.h"
#include "arraylist.h"
#include "collider.h"


typedef void (*OnTrigger)(Entity trigger, Entity other);
typedef void (*OnStay)(Entity trigger, Entity other, float time_step);


typedef enum TriggerType {
    TRIGGER_LOOK,
    TRIGGER_COLLISION
} TriggerType;


typedef struct TriggerComponent {
    TriggerType type;
    float distance;
    float roi;  // fraction of FOV
    ArrayList* overlaps;
    OnTrigger on_enter;
    OnTrigger on_exit;
    OnStay on_stay;
    ColliderGroup trigger_group;
} TriggerComponent;


typedef struct TriggerParameters {
    TriggerType type;
    float distance;
    float roi;
    OnTrigger on_enter;
    OnTrigger on_exit;
    OnStay on_stay;
    ColliderGroup trigger_group;
} TriggerParameters;


typedef enum TriggerEffectType {
    TRIGGER_EFFECT_NONE,
    TRIGGER_EFFECT_SOUND,
} TriggerEffectType;


typedef struct TriggerEffect {
    TriggerEffectType type;
    String name;
    struct TriggerEffect* next;
} TriggerEffect;


TriggerComponent* TriggerComponent_add(Entity entity, TriggerParameters params);

void TriggerComponent_remove(Entity entity);
