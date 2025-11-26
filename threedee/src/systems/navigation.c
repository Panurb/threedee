#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "systems/navigation.h"
#include "util.h"
#include "heap.h"
#include "raycast.h"
#include "list.h"
#include "scene.h"
#include "render.h"


int create_waypoint(Vector3 pos) {
    int i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(pos.x, pos.y + 1.0f, pos.z),
    });
    WaypointComponent_add(i);

    return i;
}


void reconstruct_path(int current, List* path) {
    while(current != -1) {
        List_add(path, current);
        WaypointComponent* waypoint = get_component(current, COMPONENT_WAYPOINT);
        current = waypoint->came_from;
    }
}


float heuristic(int start, int goal) {
    return dist3(get_position(start), get_position(goal));
}


bool a_star(int start, int goal, List* path) {
    Heap* open_set = Heap_create();

    Heap_insert(open_set, start);

    List_clear(path);

    for (int i = 0; i < scene->components->entities; i++) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        if (waypoint) {
            waypoint->came_from = -1;
            waypoint->g_score = INFINITY;
            waypoint->f_score = INFINITY;
        }
    }

    WaypointComponent* start_waypoint = get_component(start, COMPONENT_WAYPOINT);
    start_waypoint->g_score = 0.0;
    start_waypoint->f_score = heuristic(start, goal);

    while (open_set->size > 0) {
        int current = Heap_extract(open_set);

        if (current == goal) {
            reconstruct_path(current, path);
            Heap_destroy(open_set);
            return true;
        }

        WaypointComponent* waypoint = get_component(current, COMPONENT_WAYPOINT);

        for (ListNode* node = waypoint->neighbors->head; node; node = node->next) {
            int n = node->value;

            WaypointComponent* neighbor = get_component(n, COMPONENT_WAYPOINT);

            // Waypoint component may have been removed or entity destroyed
            if (!neighbor) continue;

            float d = heuristic(current, n);

            float tentative_g_score = waypoint->g_score + d;

            if (tentative_g_score < neighbor->g_score) {
                neighbor->came_from = current;
                neighbor->g_score = tentative_g_score;
                neighbor->f_score = neighbor->g_score + heuristic(n, goal);

                if (Heap_find(open_set, n) == -1) {
                    Heap_insert(open_set, n);
                }
            }
        }
    }

    Heap_destroy(open_set);
    return false;
}


float connection_distance(int i, int j) {
    Vector3 a = get_position(i);
    Vector3 b = get_position(j);
    Vector3 v = sub3(b, a);
    float d = norm3(v);

    WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
    if (d > waypoint->range) {
        return 0.0f;
    }

    Ray ray = {
        .origin = a,
        .direction = normalized3(v),
        .range = d
    };
    Hit info_a = raycast(ray, GROUP_WALLS);

    if (info_a.entity != NULL_ENTITY) {
        LOG_DEBUG("Connection between %d and %d blocked by entity %d", i, j, info_a.entity);
        return 0.0f;
    }

    return d;
}


void update_connection(int i, int n) {
    LOG_DEBUG("Updating connection between %d and %d", i, n);
    float d = connection_distance(i, n);
    LOG_DEBUG("Distance between %d and %d: %f", i, n, d);
    if (d > 0.0f) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        WaypointComponent* neighbor = get_component(n, COMPONENT_WAYPOINT);
        List_add(waypoint->neighbors, n);

        if (!List_find(neighbor->neighbors, i)) {
            List_add(neighbor->neighbors, i);
        }
    }
}


void init_waypoints() {
    for (int i = 0; i < scene->components->entities; i++) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        if (!waypoint) continue;

        for (int j = 0; j < scene->components->entities; j++) {
            if (i == j) continue;
            WaypointComponent* n = get_component(j, COMPONENT_WAYPOINT);
            if (!n) continue;

            update_connection(i, j);
        }
    }
}


void update_waypoints() {
    for (int i = 0; i < scene->components->entities; i++) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        if (!waypoint) continue;

        List_clear(waypoint->neighbors);
    }

    for (int i = 0; i < scene->components->entities; i++) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        if (!waypoint) continue;

        for (int j = 0; j < scene->components->entities; j++) {
            if (i == j) continue;
            WaypointComponent* n = get_component(j, COMPONENT_WAYPOINT);
            if (!n) continue;
            if (!entity_is_dynamic(j)) continue;

            update_connection(i, j);
        }
    }
}


void draw_waypoints() {
    for (int i = 0; i < scene->components->entities; i++) {
        WaypointComponent* waypoint = get_component(i, COMPONENT_WAYPOINT);
        if (!waypoint) continue;

        Vector3 pos = get_position(i);

        draw_circle(pos, 0.1f, 8, COLOR_WHITE);

        for (ListNode* node = waypoint->neighbors->head; node; node = node->next) {
            int k = node->value;
            Color color = COLOR_WHITE;
            color.a = 0.25f;
            draw_line(pos, get_position(k), 0.04f, color);
        }
    }
}
