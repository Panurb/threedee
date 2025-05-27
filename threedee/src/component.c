#define _USE_MATH_DEFINES

#include <stdbool.h>
#include <stdlib.h>

#include "component.h"
#include "util.h"
#include "input.h"
#include "scene.h"


ComponentData* ComponentData_create() {
    ComponentData* components = malloc(sizeof(ComponentData));
    components->entities = 0;
    components->added_entities = NULL;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        components->coordinate[i] = NULL;
        components->camera[i] = NULL;
    }
    return components;
}


TransformComponent* CoordinateComponent_add(int entity, Vector2 pos, float angle) {
    TransformComponent* coord = malloc(sizeof(TransformComponent));
    coord->position = pos;
    coord->angle = angle;
    coord->parent = -1;
    coord->children = List_create();
    coord->lifetime = -1.0f;
    coord->prefab[0] = '\0';
    coord->scale = ones2();
    coord->previous.position = pos;
    coord->previous.angle = coord->angle;
    coord->previous.scale = ones2();

    game_data->components->coordinate[entity] = coord;

    return coord;
}


TransformComponent* CoordinateComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->coordinate[entity];
}


void CoordinateComponent_remove(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    if (coord) {
        // if (coord->parent != -1) {
        //     List_remove(CoordinateComponent_get(coord->parent)->children, entity);
        // }
        for (ListNode* node = coord->children->head; node; node = node->next) {
            TransformComponent* child = CoordinateComponent_get(node->value);
            if (child) {
                child->parent = -1;
            }
        }
        List_delete(coord->children);
        free(coord);
        game_data->components->coordinate[entity] = NULL;
    }
}


CameraComponent* CameraComponent_add(int entity, Resolution resolution, float fov) {
    CameraComponent* camera = malloc(sizeof(CameraComponent));

    camera->resolution = resolution;
    camera->fov = fov;

    camera->projection_matrix = perspective_projection_matrix(
        camera->fov, (float)camera->resolution.w / (float)camera->resolution.h, 0.1f, 1000.0f
    );

    game_data->components->camera[entity] = camera;
    return camera;
}


CameraComponent* CameraComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->camera[entity];
}


void CameraComponent_remove(int entity) {
    CameraComponent* camera = CameraComponent_get(entity);
    if (camera) {
        free(camera);
        game_data->components->camera[entity] = NULL;
    }
}


SoundComponent* SoundComponent_add(int entity, Filename hit_sound) {
    SoundComponent* sound = malloc(sizeof(SoundComponent));
    sound->size = 4;
    for (int i = 0; i < sound->size; i++) {
        sound->events[i] = NULL;
    }
    strcpy(sound->hit_sound, hit_sound);
    strcpy(sound->loop_sound, "");
    game_data->components->sound[entity] = sound;
    return sound;
}


SoundComponent* SoundComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->sound[entity];
}


void SoundComponent_remove(int entity) {
    SoundComponent* sound = SoundComponent_get(entity);
    if (sound) {
        free(sound);
        game_data->components->sound[entity] = NULL;
    }
}


int create_entity() {
    for (int i = 0; i < game_data->components->entities; i++) {
        if (!game_data->components->coordinate[i]) {
            if (game_data->components->added_entities) {
                List_add(game_data->components->added_entities, i);
            }
            return i;
        }
    }

    game_data->components->entities++;
    if (game_data->components->added_entities) {
        List_add(game_data->components->added_entities, game_data->components->entities - 1);
    }
    return game_data->components->entities - 1;
}


int get_root(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    if (coord->parent != -1) {
        return get_root(coord->parent);
    }
    return entity;
}


void add_child(int parent, int child) {
    CoordinateComponent_get(child)->parent = parent;
    List_append(CoordinateComponent_get(parent)->children, child);
}


void remove_children(int parent) {
    TransformComponent* coord = CoordinateComponent_get(parent);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        CoordinateComponent_get(node->value)->parent = -1;
    }
    List_clear(coord->children);
}


void remove_parent(int child) {
    TransformComponent* coord = CoordinateComponent_get(child);
    if (coord->parent != NULL_ENTITY) {
        List_remove(CoordinateComponent_get(coord->parent)->children, child);
        coord->parent = NULL_ENTITY;
    }
}


void remove_prefab(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    coord->prefab[0] = '\0';
}


void destroy_entity(int entity) {
    if (entity == -1) return;

    // TODO: remove parent

    CoordinateComponent_remove(entity);
    CameraComponent_remove(entity);

    if (entity == game_data->components->entities - 1) {
        game_data->components->entities--;
    }
}


void destroy_entities(List* entities) {
    ListNode* node;
    FOREACH(node, entities) {
        destroy_entity(node->value);
    }
}


void do_destroy_entity_recursive(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        do_destroy_entity_recursive(node->value);
    }
    List_clear(coord->children);
    destroy_entity(entity);
}


void destroy_entity_recursive(int entity) {
    remove_parent(entity);
    do_destroy_entity_recursive(entity);
}


void ComponentData_clear() {
    for (int i = 0; i < game_data->components->entities; i++) {
        destroy_entity(i);
    }
    game_data->components->entities = 0;
}


Matrix3 get_transform(int entity) {
    // CoordinateComponent* coord = CoordinateComponent_get(entity);
    // Matrix3 transform = transform_matrix(coord->position, coord->angle, coord->scale);
    // if (coord->parent != -1) {
    //     return matrix3_mult(get_transform(coord->parent), transform);
    // }
    return (Matrix3) { 1.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f,
                     CoordinateComponent_get(entity)->position.x,
                     CoordinateComponent_get(entity)->position.y,
                     1.0f };
}


Vector2 get_xy(int entity) {
    Matrix3 transform = get_transform(entity);
    return position_from_transform(transform);
}


float get_angle(int entity) {
    Matrix3 transform = get_transform(entity);
    return angle_from_transform(transform);
}


Vector2 get_scale(int entity) {
    Matrix3 transform = get_transform(entity);
    return scale_from_transform(transform);
}


Vector2 get_position_interpolated(int entity, float delta) {
    Vector2 previous_position = CoordinateComponent_get(entity)->previous.position;
    Vector2 current_position = get_xy(entity);

    float x = lerp(previous_position.x, current_position.x, delta);
    float y = lerp(previous_position.y, current_position.y, delta);

    return (Vector2) { x, y };
}


float get_angle_interpolated(int entity, float delta) {
    float previous_angle = CoordinateComponent_get(entity)->previous.angle;
    float current_angle = get_angle(entity);

    return lerp_angle(previous_angle, current_angle, delta);
}


Vector2 get_scale_interpolated(int entity, float delta) {
    Vector2 previous_scale = CoordinateComponent_get(entity)->previous.scale;
    Vector2 current_scale = get_scale(entity);

    float x = lerp(previous_scale.x, current_scale.x, delta);
    float y = lerp(previous_scale.y, current_scale.y, delta);

    return (Vector2) { x, y };
}


bool entity_exists(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    if (coord) {
        return true;
    }
    return false;
}


int get_parent(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    return coord->parent;
}


List* get_children(int entity) {
    TransformComponent* coord = CoordinateComponent_get(entity);
    return coord->children;
}


Vector2 get_entities_center(List* entities) {
    Vector2 center = zeros2();
    ListNode* node;
    FOREACH(node, entities) {
        int i = node->value;
        if (CoordinateComponent_get(i)->parent == -1) {
            center = sum(center, get_xy(i));
        }
    }
    if (entities->size != 0) {
        center = mult(1.0f / entities->size, center);
    }

    return center;
}
