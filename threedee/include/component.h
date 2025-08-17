#pragma once

#include <stdbool.h>

#include "util.h"
#include "list.h"
#include "linalg.h"
#include "components/camera.h"
#include "components/collider.h"
#include "components/controller.h"
#include "components/enemy.h"
#include "components/light.h"
#include "components/mesh.h"
#include "components/player.h"
#include "components/rigidbody.h"
#include "components/sound.h"
#include "components/transform.h"
#include "components/waypoint.h"
#include "components/weather.h"

#define MAX_ENTITIES 2000


typedef struct ComponentData {
    int entities;
    List* added_entities;
    TransformComponent* transform[MAX_ENTITIES];
    CameraComponent* camera[MAX_ENTITIES];
    SoundComponent* sound[MAX_ENTITIES];
    MeshComponent* mesh[MAX_ENTITIES];
    LightComponent* light[MAX_ENTITIES];
    RigidBodyComponent* rigid_body[MAX_ENTITIES];
    ColliderComponent* collider[MAX_ENTITIES];
    ControllerComponent* controller[MAX_ENTITIES];
    WeatherComponent* weather[MAX_ENTITIES];
    PlayerComponent* player[MAX_ENTITIES];
    WaypointComponent* waypoint[MAX_ENTITIES];
    EnemyComponent* enemy[MAX_ENTITIES];
} ComponentData;

typedef enum ComponentType {
    COMPONENT_TRANSFORM,
    COMPONENT_CAMERA,
    COMPONENT_SOUND,
    COMPONENT_MESH,
    COMPONENT_LIGHT,
    COMPONENT_RIGIDBODY,
    COMPONENT_COLLIDER,
    COMPONENT_CONTROLLER,
    COMPONENT_WEATHER,
    COMPONENT_PLAYER,
    COMPONENT_WAYPOINT,
    COMPONENT_ENEMY
} ComponentType;

ComponentData* ComponentData_create();

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
void set_transform(Entity entity, Matrix4 transform);
Vector3 get_position(Entity entity);
Vector2 get_xy(Entity entity);
Quaternion get_rotation(Entity entity);
float get_yaw(Entity entity);
Vector3 get_scale(Entity entity);

Vector2 get_position_interpolated(int entity, float delta);
float get_angle_interpolated(int entity, float delta);
Vector2 get_scale_interpolated(int entity, float delta);

bool entity_exists(Entity entity);
bool entity_is_dynamic(Entity entity);

int get_parent(Entity entity);
List* get_children(Entity entity);

Vector3 get_entities_center(List* entities);

void look_at(Entity entity, Vector3 target);
void turn_to(Entity entity, Vector3 direction, float turn_speed);
