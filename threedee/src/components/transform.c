#include "components/transform.h"
#include "scene.h"


TransformComponent* TransformComponent_add(Entity entity, Vector3 pos) {
    TransformComponent* trans = malloc(sizeof(TransformComponent));
    trans->position = pos;
    trans->rotation = (Quaternion) { 0.0f, 0.0f, 0.0f, 1.0f };
    trans->scale = ones3();
    trans->parent = NULL_ENTITY;
    trans->children = List_create();
    trans->lifetime = -1.0f;
    trans->prefab[0] = '\0';
    trans->previous.position = pos;
    trans->previous.rotation = trans->rotation;
    trans->previous.scale = ones3();

    scene->components->transform[entity] = trans;

    return trans;
}


void TransformComponent_remove(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    if (coord) {
        // if (coord->parent != -1) {
        //     List_remove(CoordinateComponent_get(coord->parent)->children, entity);
        // }
        for (ListNode* node = coord->children->head; node; node = node->next) {
            TransformComponent* child = get_component(node->value, COMPONENT_TRANSFORM);
            if (child) {
                child->parent = -1;
            }
        }
        List_delete(coord->children);
        free(coord);
        scene->components->transform[entity] = NULL;
    }
}