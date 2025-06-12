#define _USE_MATH_DEFINES

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "input.h"
#include "scene.h"
#include "component.h"
#include "components/light.h"
#include "components/rigidbody.h"
#include "components/transform.h"


ComponentData* ComponentData_create() {
    ComponentData* components = malloc(sizeof(ComponentData));
    components->entities = 0;
    components->added_entities = NULL;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        components->transform[i] = NULL;
        components->camera[i] = NULL;
        components->sound[i] = NULL;
        components->mesh[i] = NULL;
        components->light[i] = NULL;
        components->rigid_body[i] = NULL;
        components->collider[i] = NULL;
    }
    return components;
}


SoundComponent* SoundComponent_add(Entity entity, Filename hit_sound) {
    SoundComponent* sound = malloc(sizeof(SoundComponent));
    sound->size = 4;
    for (int i = 0; i < sound->size; i++) {
        sound->events[i] = NULL;
    }
    strcpy(sound->hit_sound, hit_sound);
    strcpy(sound->loop_sound, "");
    scene->components->sound[entity] = sound;
    return sound;
}


SoundComponent* SoundComponent_get(Entity entity) {
    if (entity == -1) return NULL;
    return scene->components->sound[entity];
}


void SoundComponent_remove(Entity entity) {
    SoundComponent* sound = SoundComponent_get(entity);
    if (sound) {
        free(sound);
        scene->components->sound[entity] = NULL;
    }
}


Entity create_entity() {
    for (Entity i = 0; i < scene->components->entities; i++) {
        if (!scene->components->transform[i]) {
            if (scene->components->added_entities) {
                List_add(scene->components->added_entities, i);
            }
            return i;
        }
    }

    scene->components->entities++;
    if (scene->components->added_entities) {
        List_add(scene->components->added_entities, scene->components->entities - 1);
    }
    return scene->components->entities - 1;
}


void* get_component(Entity entity, ComponentType component_type) {
    if (entity == NULL_ENTITY) {
        return NULL;
    }

    switch (component_type) {
        case COMPONENT_TRANSFORM:
            return scene->components->transform[entity];
        case COMPONENT_CAMERA:
            return scene->components->camera[entity];
        case COMPONENT_SOUND:
            return SoundComponent_get(entity);
        case COMPONENT_MESH:
            return scene->components->mesh[entity];
        case COMPONENT_LIGHT:
            return scene->components->light[entity];
        case COMPONENT_RIGIDBODY:
            return scene->components->rigid_body[entity];
        case COMPONENT_COLLIDER:
            return scene->components->collider[entity];
        default:
            LOG_ERROR("Unknown component type: %d", component_type);
            return NULL;
    }
}


void remove_component(Entity entity, ComponentType component_type) {
    switch (component_type) {
        case COMPONENT_TRANSFORM:
            TransformComponent_remove(entity);
            break;
        case COMPONENT_CAMERA:
            CameraComponent_remove(entity);
            break;
        case COMPONENT_SOUND:
            SoundComponent_remove(entity);
            break;
        case COMPONENT_MESH:
            MeshComponent_remove(entity);
            break;
        case COMPONENT_LIGHT:
            LightComponent_remove(entity);
            break;
        case COMPONENT_RIGIDBODY:
            RigidBodyComponent_remove(entity);
        case COMPONENT_COLLIDER:
            ColliderComponent_remove(entity);
            break;
        default:
            LOG_ERROR("Unknown component type: %d", component_type);
            break;
    }
}


Entity get_root(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    if (coord->parent != -1) {
        return get_root(coord->parent);
    }
    return entity;
}


void add_child(Entity parent, Entity child) {
    TransformComponent* trans = get_component(child, COMPONENT_TRANSFORM);
    trans->parent = parent;
    TransformComponent* parent_trans = get_component(parent, COMPONENT_TRANSFORM);
    List_append(parent_trans->children, child);
}


void remove_children(Entity parent) {
    TransformComponent* trans = get_component(parent, COMPONENT_TRANSFORM);
    for (ListNode* node = trans->children->head; node; node = node->next) {
        TransformComponent* child_trans = get_component(node->value, COMPONENT_TRANSFORM);
        child_trans->parent = NULL_ENTITY;
    }
    List_clear(trans->children);
}


void remove_parent(Entity child) {
    TransformComponent* trans = get_component(child, COMPONENT_TRANSFORM);
    if (trans->parent != NULL_ENTITY) {
        TransformComponent* parent = get_component(trans->parent, COMPONENT_TRANSFORM);
        List_remove(parent->children, child);
        trans->parent = NULL_ENTITY;
    }
}


void remove_prefab(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    coord->prefab[0] = '\0';
}


void destroy_entity(Entity entity) {
    if (entity == -1) return;

    // TODO: remove parent

    TransformComponent_remove(entity);
    CameraComponent_remove(entity);

    if (entity == scene->components->entities - 1) {
        scene->components->entities--;
    }
}


void destroy_entities(List* entities) {
    ListNode* node;
    FOREACH(node, entities) {
        destroy_entity(node->value);
    }
}


void do_destroy_entity_recursive(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        do_destroy_entity_recursive(node->value);
    }
    List_clear(coord->children);
    destroy_entity(entity);
}


void destroy_entity_recursive(Entity entity) {
    remove_parent(entity);
    do_destroy_entity_recursive(entity);
}


void ComponentData_clear() {
    for (int i = 0; i < scene->components->entities; i++) {
        destroy_entity(i);
    }
    scene->components->entities = 0;
}


Matrix4 get_transform(Entity entity) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    Matrix4 transform = transform_matrix(trans->position, trans->rotation, trans->scale);
    if (trans->parent != NULL_ENTITY) {
        return matrix4_mult(get_transform(trans->parent), transform);
    }
    return transform;
}


Vector3 get_position(Entity entity) {
    Matrix4 transform = get_transform(entity);
    return position_from_transform(transform);
}


Vector2 get_xy(Entity entity) {
    Vector3 pos = get_position(entity);
    return (Vector2) { pos.x, pos.y };
}


Quaternion get_rotation(Entity entity) {
    Matrix4 transform = get_transform(entity);
    Matrix3 rot = {
        transform._11, transform._12, transform._13,
        transform._21, transform._22, transform._23,
        transform._31, transform._32, transform._33
    };
    return rotation_matrix_to_quaternion(rot);
}


Vector3 get_scale(Entity entity) {
    Matrix4 transform = get_transform(entity);
    return scale_from_transform(transform);
}


// Vector2 get_position_interpolated(int entity, float delta) {
//     Vector2 previous_position = TransformComponent_get(entity)->previous.position;
//     Vector2 current_position = get_xy(entity);
//
//     float x = lerp(previous_position.x, current_position.x, delta);
//     float y = lerp(previous_position.y, current_position.y, delta);
//
//     return (Vector2) { x, y };
// }
//
//
// float get_angle_interpolated(int entity, float delta) {
//     float previous_angle = TransformComponent_get(entity)->previous.angle;
//     float current_angle = get_angle(entity);
//
//     return lerp_angle(previous_angle, current_angle, delta);
// }
//
//
// Vector2 get_scale_interpolated(int entity, float delta) {
//     Vector2 previous_scale = TransformComponent_get(entity)->previous.scale;
//     Vector2 current_scale = get_scale(entity);
//
//     float x = lerp(previous_scale.x, current_scale.x, delta);
//     float y = lerp(previous_scale.y, current_scale.y, delta);
//
//     return (Vector2) { x, y };
// }


bool entity_exists(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    if (coord) {
        return true;
    }
    return false;
}


int get_parent(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    return coord->parent;
}


List* get_children(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    return coord->children;
}


Vector3 get_entities_center(List* entities) {
    Vector3 center = zeros3();
    ListNode* node;
    FOREACH(node, entities) {
        int i = node->value;
        TransformComponent* trans = get_component(i, COMPONENT_TRANSFORM);
        if (trans->parent == NULL_ENTITY) {
            center = sum3(center, get_position(i));
        }
    }
    if (entities->size != 0) {
        center = mult3(1.0f / entities->size, center);
    }

    return center;
}
