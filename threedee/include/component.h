#pragma once

#include <stdbool.h>

#include "util.h"
#include "list.h"
#include "linalg.h"

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

typedef enum {
    LAYER_GROUND,
    LAYER_FLOOR,
    LAYER_PATHS,
    LAYER_DECALS,
    LAYER_SHADOWS,
    LAYER_WALLS,
    LAYER_CORPSES,
    LAYER_ITEMS,
    LAYER_VEHICLES,
    LAYER_ENEMIES,
    LAYER_WEAPONS,
    LAYER_PLAYERS,
    LAYER_TREES,
    LAYER_PARTICLES,
    LAYER_ROOFS,
    LAYER_MENU
} Layer;

typedef struct {
    Filename filename;
    int texture_index;
    float width;
    float height;
    float shine;
    Layer layer;
    float alpha;
    float stretch;
    float stretch_speed;
    bool tile;
    Vector2 offset;
    float tile_width;
    float tile_height;
} ImageComponent;

typedef enum {
    AXIS_NONE,
    AXIS_POSITION,
    AXIS_ANGLE,
    AXIS_ALL
} AxisLock;

typedef struct {
    Vector2 velocity;
    Vector2 acceleration;
    struct {
        List* entities;
        Vector2 overlap;
        Vector2 velocity;
    } collision;
    float angular_velocity;
    float angular_acceleration;
    float mass;
    float friction;
    float bounce;
    float drag;
    float drag_sideways;
    float speed;
    float max_speed;
    float angular_drag;
    float max_angular_speed;
    bool slowed;
    bool on_ground;
    AxisLock lock;
} PhysicsComponent;

typedef enum {
    COLLIDER_CIRCLE,
    COLLIDER_RECTANGLE
} ColliderType;

typedef enum {
    GROUP_ALL,
    GROUP_WALLS,
    GROUP_ITEMS,
    GROUP_PLAYERS,
    GROUP_ENEMIES,
    GROUP_VEHICLES,
    GROUP_TREES,
    GROUP_ROADS,
    GROUP_BARRIERS,
    GROUP_BULLETS,
    GROUP_LIGHTS,
    GROUP_OBSTACLES,
    GROUP_FLOORS,
    GROUP_VISION,
    GROUP_RAYS,
    GROUP_DEBRIS,
    GROUP_DOORS,
    GROUP_ENERGY
} ColliderGroup;

typedef enum {
    TRIGGER_NONE,
    TRIGGER_WIN,
    TRIGGER_SLOW,
    TRIGGER_GROUND,
    TRIGGER_DAMAGE,
    TRIGGER_BURN,
    TRIGGER_FREEZE,
    TRIGGER_WET,
    TRIGGER_TRAP,
    TRIGGER_PUSH,
} TriggerType;

typedef struct {
    bool enabled;
    ColliderGroup group;
    ColliderType type;
    int last_collision;
    float radius;
    float width;
    float height;
    TriggerType trigger_type;
} ColliderComponent;

typedef enum {
    PLAYER_ON_FOOT,
    PLAYER_SHOOT,
    PLAYER_RELOAD,
    PLAYER_ENTER,
    PLAYER_DRIVE,
    PLAYER_PASSENGER,
    PLAYER_MENU,
    PLAYER_MENU_GRAB,
    PLAYER_MENU_DROP,
    PLAYER_AMMO_MENU,
    PLAYER_DEAD,
    PLAYER_INTERACT
} PlayerState;

typedef struct {
    int joystick;
    int buttons[12];
    bool buttons_down[12];
    bool buttons_pressed[12];
    bool buttons_released[12];
    int axes[8];
    Vector2 left_stick;
    Vector2 right_stick;
    Vector2 dpad;
    float left_trigger;
    float right_trigger;
} Controller;

typedef struct {
    int target;
    float acceleration;
    int vehicle;
    int item;
    int inventory_size;
    int inventory[4];
    int grabbed_item;
    PlayerState state;
    Controller controller;
    int ammo_size;
    int ammo[4];
    int arms;
    int money;
    float use_timer;
    float money_timer;
    int money_increment;
    bool won;
    int keys_size;
    int keys[3];
} PlayerComponent;

typedef struct {
    bool enabled;
    float range;
    float angle;
    int rays;
    Color color;
    float brightness;
    float max_brightness;
    float flicker;
    float speed;
    float time;
    int bounces;
} LightComponent;

typedef enum {
    ENEMY_IDLE,
    ENEMY_WANDER,
    ENEMY_INVESTIGATE,
    ENEMY_CHASE,
    ENEMY_ATTACK,
    ENEMY_DEAD
} EnemyState;

typedef struct {
    bool spawner;
    EnemyState state;
    float acceleration;
    int target;
    List* path;
    float fov;
    float vision_range;
    float idle_speed;
    float walk_speed;
    float run_speed;
    int weapon;
    float desired_angle;
    float attack_delay;
    float attack_timer;
    float turn_speed;
    int bounty;
    Vector2 start_position;
    bool boss;
} EnemyComponent;

typedef enum ParticleType {
    PARTICLE_NONE,
    PARTICLE_BULLET,
    PARTICLE_BLOOD,
    PARTICLE_SPARKS,
    PARTICLE_DIRT,
    PARTICLE_ROCK,
    PARTICLE_SPLINTER,
    PARTICLE_FIRE,
    PARTICLE_ENERGY,
    PARTICLE_GLASS,
    PARTICLE_SNOW,
    PARTICLE_RAIN,
    PARTICLE_STEAM,
    PARTICLE_WATER,
    PARTICLE_RADIATION,
    PARTICLE_PUSH
} ParticleType;

typedef struct {
    ParticleType type;
    bool enabled;
    bool loop;
    float angle;
    int particles;
    int max_particles;
    int first;
    float spread;
    float speed_spread;
    float speed;
    float max_time;
    Vector2 position[100];
    Vector2 velocity[100];
    float time[100];
    float start_size;
    float end_size;
    Color outer_color;
    Color inner_color;
    float rate;
    float timer;
    Vector2 origin;
    float width;
    float height;
    float stretch;
    float wind_factor;
} ParticleComponent;

typedef struct {
    bool on_road;
    float fuel;
    float max_fuel;
    float acceleration;
    float max_speed;
    float turning;
    int size;
    int riders[4];
    Vector2 seats[4];
} VehicleComponent;

typedef enum {
    AMMO_MELEE,
    AMMO_PISTOL,
    AMMO_RIFLE,
    AMMO_SHOTGUN,
    AMMO_ENERGY,
    AMMO_ROPE,
    AMMO_FLAME,
    AMMO_WATER,
    AMMO_FREEZE
} AmmoType;

typedef struct {
    float cooldown;
    float fire_rate;
    float recoil;
    float recoil_up;
    float recoil_down;
    float max_recoil;
    int magazine;
    int max_magazine;
    int damage;
    float reload_time;
    bool reloading;
    float range;
    float sound_range;
    float spread;
    int shots;
    bool automatic;
    int penetration;
    AmmoType ammo_type;
    Filename sound;
} WeaponComponent;

typedef enum ItemType {
    ITEM_NONE,
    ITEM_HEAL,
    ITEM_LIGHT,
    ITEM_LASER,
    ITEM_SILENCER,
    ITEM_HOLLOW_POINT,
    ITEM_SCOPE,
    ITEM_MAGAZINE,
    ITEM_STOCK,
    ITEM_BARREL,
    ITEM_GRIP,
    ITEM_FUEL,
    ITEM_KEY,
    ITEM_SAVE,
} ItemType;

typedef struct {
    int attachments[5];
    int size;
    int price;
    ButtonText name;
    float use_time;
    ItemType type;
    int value;
} ItemComponent;

typedef struct {
    List* neighbors;
    List* new_neighbors;
    int came_from;
    float f_score;
    float g_score;
    float range;
} WaypointComponent;

typedef enum {
    STATUS_NONE,
    STATUS_BURNING,
    STATUS_FROZEN
} StatusEffect;

typedef struct {
    bool dead;
    int health;
    int max_health;
    Filename dead_image;
    Filename decal;
    Filename die_sound;
    struct {
        StatusEffect type;
        float lifetime;
        Entity entity;
        float timer;
    } status;
} HealthComponent;

typedef struct {
    float fov;
    Resolution resolution;
    Matrix4 projection_matrix;
} CameraComponent;

typedef struct {
    int prev;
    int next;
    float width;
} PathComponent;

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

typedef struct {
    AmmoType type;
    int size;
} AmmoComponent;

typedef struct {
    int frames;
    int current_frame;
    float timer;
    float framerate;
    bool play_once;
} AnimationComponent;

static const String DOOR_KEY_NAMES[] = {
    "None",
    "Bronze Key",
    "Silver Key",
    "Gold Key"
};

typedef enum {
    KEY_NONE,
    KEY_BRONZE,
    KEY_SILVER,
    KEY_GOLD
} DoorKey;

typedef struct {
    bool locked;
    int price;
    int key;
} DoorComponent;

typedef struct {
    int parent;
    float min_length;
    float max_length;
    float strength;
    float max_angle;
} JointComponent;

typedef enum {
    WIDGET_WINDOW,
    WIDGET_CONTAINER,
    WIDGET_LABEL,
    WIDGET_BUTTON,
    WIDGET_SPINBOX,
    WIDGET_SLIDER,
    WIDGET_DROPDOWN,
    WIDGET_SCROLLBAR,
    WIDGET_TEXTBOX,
    WIDGET_CHECKBOX
} WidgetType;

typedef struct ComponentData ComponentData;

typedef void (*OnClick)(int);
typedef void (*OnChange)(int, int);

typedef struct {
    bool enabled;
    WidgetType type;
    bool selected;
    ButtonText string;
    OnClick on_click;
    OnChange on_change;
    int value;
    int min_value;
    int max_value;
    bool cyclic;
    ButtonText* strings;
} WidgetComponent;

typedef struct {
    String source_string;
    String string;
    int size;
    Color color;
} TextComponent;

typedef struct {
    void* array[MAX_ENTITIES];
    List* order;
} OrderedArray;

struct ComponentData {
    int entities;
    List* added_entities;
    TransformComponent* coordinate[MAX_ENTITIES];
    CameraComponent* camera[MAX_ENTITIES];
    SoundComponent* sound[MAX_ENTITIES];
};

ComponentData* ComponentData_create();

TransformComponent* TransformComponent_add(Entity entity, Vector3 pos);
TransformComponent* TransformComponent_get(Entity entity);
void TransformComponent_remove(Entity entity);

CameraComponent* CameraComponent_add(int entity, Resolution resolution, float fov);
CameraComponent* CameraComponent_get(int entity);
void CameraComponent_remove(int entity);

SoundComponent* SoundComponent_add(int entity, Filename hit_sound);
SoundComponent* SoundComponent_get(int entity);
void SoundComponent_remove(int entity);

int create_entity();
void destroy_entity(int i);
void destroy_entities(List* entities);
void destroy_entity_recursive(int entity);
int get_root(int entity);
void add_child(int parent, int child);
void remove_children(int parent);
void remove_parent(int child);
void remove_prefab(int entity);

void ComponentData_clear();

Matrix4 get_transform(Entity entity);
Vector3 get_position(Entity entity);
Vector2 get_xy(Entity entity);
Vector3 get_rotation(Entity entity);
Vector3 get_scale(Entity entity);

Vector2 get_position_interpolated(int entity, float delta);
float get_angle_interpolated(int entity, float delta);
Vector2 get_scale_interpolated(int entity, float delta);

bool entity_exists(int entity);

int get_parent(int entity);

List* get_children(int entity);

Vector3 get_entities_center(List* entities);
