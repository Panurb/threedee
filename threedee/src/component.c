#define _USE_MATH_DEFINES

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "../include/systems/input.h"
#include "scene.h"
#include "component.h"
#include "components/light.h"
#include "components/rigidbody.h"
#include "components/transform.h"


ComponentData* ComponentData_create() {
    ComponentData* components = malloc(sizeof(ComponentData));
    memset(components, 0, sizeof(ComponentData));
    return components;
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
            return scene->components->sound[entity];
        case COMPONENT_MESH:
            return scene->components->mesh[entity];
        case COMPONENT_LIGHT:
            return scene->components->light[entity];
        case COMPONENT_RIGIDBODY:
            return scene->components->rigid_body[entity];
        case COMPONENT_COLLIDER:
            return scene->components->collider[entity];
        case COMPONENT_CONTROLLER:
            return scene->components->controller[entity];
        case COMPONENT_WEATHER:
            return scene->components->weather[entity];
        case COMPONENT_PLAYER:
            return scene->components->player[entity];
        case COMPONENT_WAYPOINT:
            return scene->components->waypoint[entity];
        case COMPONENT_ENEMY:
            return scene->components->enemy[entity];
        case COMPONENT_SPRITE:
            return scene->components->sprite[entity];
        case COMPONENT_EMITTER:
            return scene->components->emitter[entity];
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
            break;
        case COMPONENT_COLLIDER:
            ColliderComponent_remove(entity);
            break;
        case COMPONENT_CONTROLLER:
            ControllerComponent_remove(entity);
            break;
        case COMPONENT_WEATHER:
            WeatherComponent_remove(entity);
            break;
        case COMPONENT_PLAYER:
            PlayerComponent_remove(entity);
            break;
        case COMPONENT_WAYPOINT:
            WaypointComponent_remove(entity);
            break;
        case COMPONENT_ENEMY:
            EnemyComponent_remove(entity);
            break;
        case COMPONENT_SPRITE:
            SpriteComponent_remove(entity);
            break;
        case COMPONENT_EMITTER:
            EmitterComponent_remove(entity);
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
    if (entity == NULL_ENTITY) return;

    // TODO: remove parent

    for (int i = 0; i < COMPONENT_COUNT; i++) {
        remove_component(entity, i);
    }

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
        return matrix4_mul(get_transform(trans->parent), transform);
    }
    return transform;
}


void set_transform(Entity entity, Matrix4 transform) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    trans->position = position_from_transform(transform);
    trans->scale = scale_from_transform(transform);
    trans->rotation = rotation_from_transform(transform);
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
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    Quaternion rotation = trans->rotation;
    if (trans->parent != NULL_ENTITY) {
        Quaternion parent_rotation = get_rotation(trans->parent);
        rotation = quaternion_mult(parent_rotation, rotation);
    }
    return rotation;
}


float get_yaw(Entity entity) {
    Quaternion rotation = get_rotation(entity);
    Vector3 forward = quaternion_forward(rotation);
    return atan2f(forward.y, forward.z);
}


Vector3 get_scale(Entity entity) {
    Matrix4 transform = get_transform(entity);
    return scale_from_transform(transform);
}


Matrix4 get_transform_interpolated(Entity entity, float delta) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    Vector3 position = lerp3(trans->previous.position, trans->position, delta);
    Quaternion rotation = slerp(trans->previous.rotation, trans->rotation, delta);
    Vector3 scale = lerp3(trans->previous.scale, trans->scale, delta);
    Matrix4 transform = transform_matrix(position, rotation, scale);
    if (trans->parent != NULL_ENTITY) {
        return matrix4_mul(get_transform_interpolated(trans->parent, delta), transform);
    }
    return transform;
}


Vector3 get_position_interpolated(int entity, float delta) {
    Matrix4 transform = get_transform_interpolated(entity, delta);
    return position_from_transform(transform);
}


Axes get_axes_interpolated(Entity entity, float delta) {
    Matrix4 transform = get_transform_interpolated(entity, delta);
    Axes axes;
    axes.x = vec3(transform._11, transform._21, transform._31);
    axes.y = vec3(transform._12, transform._22, transform._32);
    axes.z = vec3(transform._13, transform._23, transform._33);
    axes.right = axes.x;
    axes.up = axes.y;
    axes.forward = neg3(axes.z);
    axes.left = neg3(axes.x);
    axes.down = neg3(axes.y);
    axes.back = axes.z;
    return axes;
}


bool entity_exists(Entity entity) {
    TransformComponent* coord = get_component(entity, COMPONENT_TRANSFORM);
    if (coord) {
        return true;
    }
    return false;
}


bool entity_is_dynamic(Entity entity) {
    RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
    if (rb && !rb->asleep && rb->inv_mass > 0.0f) {
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
            center = add3(center, get_position(i));
        }
    }
    if (entities->size != 0) {
        center = mul3(1.0f / entities->size, center);
    }

    return center;
}


Axes get_axes(Entity entity) {
    Matrix4 transform = get_transform(entity);
    Axes axes;
    axes.x = vec3(transform._11, transform._21, transform._31);
    axes.y = vec3(transform._12, transform._22, transform._32);
    axes.z = vec3(transform._13, transform._23, transform._33);
    axes.right = axes.x;
    axes.up = axes.y;
    axes.forward = neg3(axes.z);
    axes.left = neg3(axes.x);
    axes.down = neg3(axes.y);
    axes.back = axes.z;
    return axes;
}


Vector3 local_to_world(Entity entity, Vector3 local_point) {
    Matrix4 transform = get_transform(entity);
    Vector4 v = map4(transform, vec4(local_point.x, local_point.y, local_point.z, 1.0f));
    return vec3(v.x, v.y, v.z);
}


void move_to(Entity entity, Vector3 target, float speed, float time_step) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    Vector3 position = get_position(entity);
    Vector3 direction = sub3(target, position);
    float distance = norm3(direction);
    if (distance < 0.01f) {
        return;
    }
    direction = normalized3(direction);
    float move_distance = speed * time_step;
    if (move_distance > distance) {
        move_distance = distance;
    }
    trans->position  = add3(position, mul3(move_distance, direction));
}


void look_at(Entity entity, Vector3 target) {
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);
    Vector3 position = trans->position;
    Vector3 direction = sub3(target, position);
    Vector3 up = vec3(0.0f, 1.0f, 0.0f);
    if (fabsf(dot3(direction, up)) > 0.99f) {
        // If the direction is almost vertical, use a different up vector
        up = vec3(0.0f, 0.0f, 1.0f);
    }
    Matrix3 rot = look_at_rotation_matrix(position, target, up);
    trans->rotation = rotation_matrix_to_quaternion(rot);
}


void turn_to(Entity entity, Vector3 target, float turn_speed, float time_step) {
    // FIXME: turns at different speed for yaw/pitch
    TransformComponent* trans = get_component(entity, COMPONENT_TRANSFORM);

    Vector3 position = trans->position;
    Vector3 direction = sub3(target, position);
    Quaternion current_rotation = trans->rotation;
    Quaternion target_rotation = quaternion_from_forward(direction, vec3_up());

    float angle_diff = quaternion_angle(current_rotation, target_rotation);

    float delta_angle = turn_speed * time_step;
    float t = fminf(1.0f, delta_angle / angle_diff);
    trans->rotation = slerp(current_rotation, target_rotation, t);
}
