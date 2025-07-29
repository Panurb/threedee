#include <stdlib.h>

#include "components/waypoint.h"
#include "scene.h"
#include "list.h"


WaypointComponent* WaypointComponent_add(Entity entity) {
    WaypointComponent* waypoint = malloc(sizeof(WaypointComponent));
    waypoint->came_from = -1;
    waypoint->g_score = INFINITY;
    waypoint->f_score = INFINITY;
    waypoint->neighbors = List_create();
    waypoint->new_neighbors = List_create();
    waypoint->range = 10.0f;

    scene->components->waypoint[entity] = waypoint;

    return waypoint;
}


void WaypointComponent_remove(Entity entity) {
    WaypointComponent* waypoint = get_component(entity, COMPONENT_WAYPOINT);
    if (waypoint) {
        ListNode* node;
        FOREACH(node, waypoint->neighbors) {
            int n = node->value;
            WaypointComponent* neighbor = get_component(n, COMPONENT_WAYPOINT);
            if (neighbor) {
                List_remove(neighbor->neighbors, entity);
            }
        }
        List_delete(waypoint->neighbors);

        FOREACH(node, waypoint->new_neighbors) {
            int n = node->value;
            WaypointComponent* neighbor = get_component(n, COMPONENT_WAYPOINT);
            if (neighbor) {
                List_remove(neighbor->new_neighbors, entity);
            }
        }
        List_delete(waypoint->new_neighbors);

        free(waypoint);
        scene->components->waypoint[entity] = NULL;
    }
}
