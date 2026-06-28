#include "components/transform.h"
#include "scene.h"


TransformComponent* TransformComponent_add(Entity entity, TransformParameters params) {
    TransformComponent* trans = malloc(sizeof(TransformComponent));
    trans->position = params.position;
    if (params.yaw != 0.0f) {
        trans->rotation = axis_angle_to_quaternion(vec3_up(), to_radians(params.yaw));
    } else {
        trans->rotation = quaternion_non_zero(params.rotation) ? params.rotation : quaternion_id();
    }
    trans->scale = non_zero3(params.scale) ? params.scale : ones3();
    trans->parent = NULL_ENTITY;
    trans->children = List_create();
    trans->lifetime = -1.0f;
    trans->prefab[0] = '\0';
    trans->previous.position = trans->position;
    trans->previous.rotation = trans->rotation;
    trans->previous.scale = trans->scale;

    scene->components->transform[entity] = trans;

    if (params.parent) {
        add_child(params.parent, entity);
    }

    return trans;
}


void TransformComponent_remove(Entity entity) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    if (trans) {
        // if (coord->parent != -1) {
        //     List_remove(CoordinateComponent_get(coord->parent)->children, entity);
        // }
        for (ListNode* node = trans->children->head; node; node = node->next) {
            TransformComponent* child = get_component(node->value, COMPONENT_TRANSFORM);
            if (child) {
                child->parent = -1;
            }
        }
        List_delete(trans->children);
        free(trans);
        scene->components->transform[entity] = NULL;
    }
}