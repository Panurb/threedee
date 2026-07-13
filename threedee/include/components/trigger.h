#pragma once

#include "util.h"
#include "arraylist.h"
#include "collider.h"


typedef void (*OnTrigger)(Entity trigger, Entity other);
typedef void (*OnStay)(Entity trigger, Entity other, float time_step);


typedef enum TriggerType {
    TRIGGER_LOOK,
    TRIGGER_COLLISION,
    TRIGGER_MANUAL,
    TRIGGER_TIMER,
    TRIGGER_PERIODIC
} TriggerType;


typedef struct TriggerComponent {
    TriggerType type;
    float distance;
    float roi;  // fraction of FOV
    ArrayList* entities;
    OnTrigger on_enter;
    OnTrigger on_exit;
    OnStay on_stay;
    ColliderGroup trigger_group;
    Entity target_entity;
    int level;
    float timer;
    float max_time;
} TriggerComponent;


typedef struct TriggerParameters {
    TriggerType type;
    float distance;
    float roi;
    OnTrigger on_enter;
    OnTrigger on_exit;
    OnStay on_stay;
    ColliderGroup trigger_group;
    Entity target_entity;
    int level;
    float max_time;
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
