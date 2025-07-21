#include "components/transform.h"
#include "scene.h"


TransformComponent* TransformComponent_add(Entity entity, TransformParameters params) {
    TransformComponent* trans = malloc(sizeof(TransformComponent));
    trans->position = params.position;
    trans->rotation = quaternion_non_zero(params.rotation) ? params.rotation : quaternion_id();
    trans->scale = non_zero3(params.scale) ? params.scale : ones3();
    trans->parent = NULL_ENTITY;
    trans->children = List_create();
    trans->lifetime = -1.0f;
    trans->prefab[0] = '\0';
    trans->previous.position = trans->position;
    trans->previous.rotation = trans->rotation;
    trans->previous.scale = trans->scale;

    if (params.parent) {
        add_child(params.parent, entity);
    }

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