#include <stdlib.h>

#include "components/collider.h"
#include "scene.h"
#include "util.h"


void ColliderComponent_add(Entity entity, ColliderType type, float radius) {
    ColliderComponent* collider = malloc(sizeof(ColliderComponent));
    collider->type = type;
    collider->radius = radius;
    collider->collisions = ArrayList_create(sizeof(Collision));
    scene->components->collider[entity] = collider;
}


void ColliderComponent_remove(Entity entity) {
    ColliderComponent* collider = scene->components->collider[entity];
    if (collider) {
        free(collider);
        scene->components->collider[entity] = NULL;
    }
}
