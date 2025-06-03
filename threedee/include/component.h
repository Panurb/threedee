#pragma once

#include <stdbool.h>

#include "util.h"
#include "list.h"
#include "linalg.h"
#include "components/mesh.h"

#define MAX_ENTITIES 2000


typedef int Entity;
#define NULL_ENTITY -1


typedef struct {
    Vector3 position;
    Rotation rotation;
    Vector3 scale;
    Entity parent;
    List* children;
    float lifetime;
    Filename prefab;
    struct {
        Vector3 position;
        Rotation rotation;
        Vector3 scale;
    } previous;
} TransformComponent;

typedef struct {
    float fov;
    float near_plane;
    float far_plane;
    Resolution resolution;
    Matrix4 projection_matrix;
} CameraComponent;

typedef struct {
    bool loop;
    int channel;
    float volume;
    float pitch;
    Filename filename;
} SoundEvent;

typedef struct {
    int size;
    SoundEvent* events[4];
    Filename hit_sound;
    Filename loop_sound;
} SoundComponent;

typedef struct ComponentData {
    int entities;
    List* added_entities;
    TransformComponent* coordinate[MAX_ENTITIES];
    CameraComponent* camera[MAX_ENTITIES];
    SoundComponent* sound[MAX_ENTITIES];
    MeshComponent* mesh[MAX_ENTITIES];
} ComponentData;

typedef enum ComponentType {
    COMPONENT_TRANSFORM,
    COMPONENT_CAMERA,
    COMPONENT_SOUND,
    COMPONENT_MESH,
} ComponentType;

ComponentData* ComponentData_create();

TransformComponent* TransformComponent_add(Entity entity, Vector3 pos);
TransformComponent* TransformComponent_get(Entity entity);
void TransformComponent_remove(Entity entity);

CameraComponent* CameraComponent_add(Entity entity, Resolution resolution, float fov);
CameraComponent* CameraComponent_get(Entity entity);
void CameraComponent_remove(Entity entity);

SoundComponent* SoundComponent_add(Entity entity, Filename hit_sound);
SoundComponent* SoundComponent_get(Entity entity);
void SoundComponent_remove(Entity entity);

void* add_component(Entity entity, ComponentType component_type);
void* get_component(Entity entity, ComponentType component_type);
void remove_component(Entity entity, ComponentType component_type);

Entity create_entity();
void destroy_entity(Entity i);
void destroy_entities(List* entities);
void destroy_entity_recursive(Entity entity);
Entity get_root(Entity entity);
void add_child(Entity parent, Entity child);
void remove_children(Entity parent);
void remove_parent(Entity child);
void remove_prefab(Entity entity);

void ComponentData_clear();

Matrix4 get_transform(Entity entity);
Vector3 get_position(Entity entity);
Vector2 get_xy(Entity entity);
Vector3 get_rotation(Entity entity);
Vector3 get_scale(Entity entity);

Vector2 get_position_interpolated(int entity, float delta);
float get_angle_interpolated(int entity, float delta);
Vector2 get_scale_interpolated(int entity, float delta);

bool entity_exists(Entity entity);

int get_parent(Entity entity);

List* get_children(Entity entity);

Vector3 get_entities_center(List* entities);
