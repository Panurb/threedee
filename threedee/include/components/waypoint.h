#pragma once


#include "util.h"
#include "list.h"


typedef struct WaypointComponent {
    List* neighbors;
    int came_from;
    float f_score;
    float g_score;
    float range;
} WaypointComponent;


WaypointComponent* WaypointComponent_add(Entity entity);


void WaypointComponent_remove(Entity entity);
