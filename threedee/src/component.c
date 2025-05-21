#define _USE_MATH_DEFINES

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "app.h"
#include "component.h"
#include "image.h"
#include "util.h"
#include "grid.h"
#include "input.h"
#include "game.h"


ComponentData* ComponentData_create() {
    ComponentData* components = malloc(sizeof(ComponentData));
    components->entities = 0;
    components->added_entities = NULL;

    components->image.order = List_create();
    components->player.order = List_create();
    components->widget.order = List_create();

    for (int i = 0; i < MAX_ENTITIES; i++) {
        components->coordinate[i] = NULL;
        components->image.array[i] = NULL;
        components->physics[i] = NULL;
        components->collider[i] = NULL;
        components->player.array[i] = NULL;
        components->light[i] = NULL;
        components->enemy[i] = NULL;
        components->particle[i] = NULL;
        components->vehicle[i] = NULL;
        components->weapon[i] = NULL;
        components->item[i] = NULL;
        components->waypoint[i] = NULL;
        components->health[i] = NULL;
        components->camera[i] = NULL;
        components->path[i] = NULL;
        components->sound[i] = NULL;
        components->ammo[i] = NULL;
        components->animation[i] = NULL;
        components->door[i] = NULL;
        components->joint[i] = NULL;
        components->widget.array[i] = NULL;
        components->text[i] = NULL;
    }
    return components;
}


CoordinateComponent* CoordinateComponent_add(int entity, Vector2f pos, float angle) {
    CoordinateComponent* coord = malloc(sizeof(CoordinateComponent));
    coord->position = pos;
    coord->angle = angle;
    coord->parent = -1;
    coord->children = List_create();
    coord->lifetime = -1.0f;
    coord->prefab[0] = '\0';
    coord->scale = ones();
    coord->previous.position = pos;
    coord->previous.angle = coord->angle;
    coord->previous.scale = ones();

    game_data->components->coordinate[entity] = coord;

    return coord;
}


CoordinateComponent* CoordinateComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->coordinate[entity];
}


void CoordinateComponent_remove(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    if (coord) {
        // if (coord->parent != -1) {
        //     List_remove(CoordinateComponent_get(coord->parent)->children, entity);
        // }
        for (ListNode* node = coord->children->head; node; node = node->next) {
            CoordinateComponent* child = CoordinateComponent_get(node->value);
            if (child) {
                child->parent = -1;
            }
        }
        List_delete(coord->children);
        free(coord);
        game_data->components->coordinate[entity] = NULL;
    }
}


ImageComponent* ImageComponent_add(int entity, Filename filename, float width, float height, Layer layer) {
    ImageComponent* image = malloc(sizeof(ImageComponent));
    strcpy(image->filename, filename);
    image->texture_index = -1;
    image->width = width;
    image->height = height;
    image->shine = 1.0;
    image->layer = layer;
    image->alpha = 1.0;
    image->stretch = 0.0f;
    image->stretch_speed = 0.0f;
    image->tile = false;
    image->offset = zeros();
    
    game_data->components->image.array[entity] = image;

    change_layer(entity, image->layer);
    change_texture(entity, filename, width, height);

    return image;
}


ImageComponent* ImageComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->image.array[entity];
}


void ImageComponent_remove(int entity) {
    ImageComponent* image = ImageComponent_get(entity);
    if (image) {
        free(image);
        game_data->components->image.array[entity] = NULL;

        List_remove(game_data->components->image.order, entity);
    }
}


PhysicsComponent* PhysicsComponent_add(int entity, float mass) {
    PhysicsComponent* phys = malloc(sizeof(PhysicsComponent));
    phys->velocity = zeros();
    phys->acceleration = zeros();
    phys->collision.entities = List_create();
    phys->collision.overlap = zeros();
    phys->collision.velocity = zeros();
    phys->angular_velocity = 0.0f;
    phys->angular_acceleration = 0.0f;
    phys->mass = mass;
    phys->friction = 0.0f;
    phys->bounce = 0.5f;
    phys->drag = 10.0f;
    phys->drag_sideways = 10.0f;
    phys->speed = 0.0;
    phys->max_speed = 20.0;
    phys->angular_drag = 10.0f;
    phys->max_angular_speed = 20.0 * M_PI;
    phys->slowed = false;
    phys->on_ground = false;
    phys->lock = AXIS_NONE;

    game_data->components->physics[entity] = phys;

    return phys;
}


PhysicsComponent* PhysicsComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->physics[entity];
}


void PhysicsComponent_remove(int entity) {
    PhysicsComponent* phys = PhysicsComponent_get(entity);
    if (phys) {
        List_delete(phys->collision.entities);
        free(phys);
        game_data->components->physics[entity] = NULL;
    }
}


ColliderComponent* ColliderComponent_add_circle(int entity, float radius, ColliderGroup group) {
    ColliderComponent* col = malloc(sizeof(ColliderComponent));
    col->enabled = true;
    col->trigger_type = TRIGGER_NONE;
    col->type = COLLIDER_CIRCLE;
    col->group = group;
    col->last_collision = -1;
    col->radius = radius;
    col->width = 2 * radius;
    col->height = 2 * radius;

    game_data->components->collider[entity] = col;

    return col;
}


ColliderComponent* ColliderComponent_add_rectangle(int entity, float width, float height, ColliderGroup group) {
    ColliderComponent* col = malloc(sizeof(ColliderComponent));
    col->enabled = true;
    col->trigger_type = TRIGGER_NONE;
    col->type = COLLIDER_RECTANGLE;
    col->group = group;
    col->last_collision = -1;
    // TODO: divide by 2?
    col->radius = sqrtf(width * width + height * height);
    col->width = width;
    col->height = height;

    game_data->components->collider[entity] = col;

    return col;
}


ColliderComponent* ColliderComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->collider[entity];
}


void ColliderComponent_remove(int entity) {
    ColliderComponent* col = ColliderComponent_get(entity);
    if (col) {
        free(col);
        game_data->components->collider[entity] = NULL;
    }
}


PlayerComponent* PlayerComponent_add(int entity) {
    PlayerComponent* player = malloc(sizeof(PlayerComponent));
    player->target = -1;
    player->acceleration = 20.0;
    player->vehicle = -1;
    player->item = 0;
    player->inventory_size = 4;
    fill(player->inventory, NULL_ENTITY, player->inventory_size);
    player->grabbed_item = -1;
    player->state = PLAYER_ON_FOOT;
    player->ammo_size = 4;
    fill(player->ammo, 0, player->ammo_size);
    player->arms = -1;
    player->money = 0;
    player->use_timer = 0.0f;
    player->money_increment = 0;
    player->money_timer = 0.0f;
    player->won = false;
    player->keys_size = 3;
    fill(player->keys, NULL_ENTITY, player->keys_size);

    player->controller.joystick = -1;
    
    int buttons[12] = { SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
                        SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, SDL_CONTROLLER_BUTTON_START,
                        SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
                        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN};
    memcpy(player->controller.buttons, buttons, sizeof(buttons));

    player->controller.left_stick = zeros();
    player->controller.right_stick = zeros();
    for (int i = 0; i < 12; i++) {
        player->controller.buttons_down[i] = false;
        player->controller.buttons_pressed[i] = false;
        player->controller.buttons_released[i] = false;
    }

    game_data->components->player.array[entity] = player;
    List_add(game_data->components->player.order, entity);

    return player;
}


PlayerComponent* PlayerComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->player.array[entity];
}


void PlayerComponent_remove(int entity) {
    PlayerComponent* player = PlayerComponent_get(entity);
    if (player) {
        if (player->controller.joystick != -1) {
            SDL_GameControllerClose(app.controllers[player->controller.joystick]);
            app.controllers[player->controller.joystick] = NULL;
        }
        free(player);
        game_data->components->player.array[entity] = NULL;
        List_remove(game_data->components->player.order, entity);
    }
}


LightComponent* LightComponent_add(int entity, float range, float angle, Color color, float brightness, float speed) {
    LightComponent* light = malloc(sizeof(LightComponent));
    light->enabled = true;
    light->range = range;
    light->angle = angle;
    light->rays = angle * 100;
    light->brightness = 0.0;
    light->max_brightness = brightness;

    light->color = color;

    light->flicker = 0.0;
    light->speed = speed;
    light->time = randf(0.0, 1.0);
    light->bounces = 0;

    game_data->components->light[entity] = light;

    return light;
}


LightComponent* LightComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->light[entity];
}


void LightComponent_remove(int entity) {
    LightComponent* light = LightComponent_get(entity);
    if (light) {
        free(light);
        game_data->components->light[entity] = NULL;
    }
}


EnemyComponent* EnemyComponent_add(int entity) {
    EnemyComponent* enemy = malloc(sizeof(EnemyComponent));
    enemy->state = ENEMY_IDLE;
    enemy->acceleration = 15.0f;
    enemy->target = -1;
    enemy->path = List_create();
    enemy->fov = M_PI_2;
    enemy->vision_range = 15.0f;
    enemy->idle_speed = 1.0f;
    enemy->walk_speed = 2.0f;
    enemy->run_speed = 6.0f;
    enemy->weapon = -1;
    enemy->desired_angle = get_angle(entity);
    enemy->attack_delay = 0.1f;
    enemy->attack_timer = enemy->attack_delay;
    enemy->turn_speed = 5.0f;
    enemy->spawner = false;
    enemy->bounty = 100;
    enemy->start_position = get_position(entity);
    enemy->boss = false;

    game_data->components->enemy[entity] = enemy;

    return enemy;
}


EnemyComponent* EnemyComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->enemy[entity];
}


void EnemyComponent_remove(int entity) {
    EnemyComponent* enemy = EnemyComponent_get(entity);
    if (enemy) {
        List_delete(enemy->path);
        free(enemy);
        game_data->components->enemy[entity] = NULL;
    }
}


ParticleComponent* ParticleComponent_add(int entity, float angle, float spread, 
        float start_size, float end_size, float speed, float rate, Color outer_color, Color inner_color) {
    ParticleComponent* particle = malloc(sizeof(ParticleComponent));
    particle->type = PARTICLE_NONE;
    particle->enabled = false;
    particle->loop = false;
    particle->angle = angle;
    particle->spread = spread;
    particle->particles = 0;
    particle->max_particles = 100;
    particle->first = 0;
    particle->start_size = start_size;
    particle->end_size = end_size;
    particle->speed = speed;
    particle->speed_spread = 0.5;
    particle->max_time = 0.5;
    for (int i = 0; i < particle->max_particles; i++) {
        particle->position[i] = zeros();
        particle->velocity[i] = zeros();
        particle->time[i] = 0.0;
    }
    particle->rate = rate;
    particle->timer = 0.0;
    particle->outer_color = outer_color;
    particle->inner_color = inner_color;
    particle->origin = zeros();
    particle->width = 0.0f;
    particle->height = 0.0f;
    particle->stretch = 0.1f;
    particle->wind_factor = 1.0f;

    game_data->components->particle[entity] = particle;

    return particle;
}


ParticleComponent* ParticleComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->particle[entity];
}


void ParticleComponent_remove(int entity) {
    ParticleComponent* particle = ParticleComponent_get(entity);
    if (particle) {
        free(particle);
        game_data->components->particle[entity] = NULL;
    }
}


VehicleComponent* VehicleComponent_add(int entity, float max_fuel) {
    VehicleComponent* vehicle = malloc(sizeof(VehicleComponent));
    vehicle->on_road = false;
    vehicle->max_fuel = max_fuel;
    vehicle->fuel = max_fuel;
    vehicle->acceleration = 30.0f;
    vehicle->max_speed = 10.0f;
    vehicle->turning = 0.25f * M_PI;
    vehicle->size = 4;
    for (int i = 0; i < vehicle->size; i++) {
        vehicle->riders[i] = -1;
    }
    vehicle->seats[0] = (Vector2f) { 1.5f, 0.75f };
    vehicle->seats[1] = (Vector2f) { 1.5f, -0.75f };
    vehicle->seats[2] = (Vector2f) { 0.0f, 0.75f };
    vehicle->seats[3] = (Vector2f) { 0.0f, -0.75f };

    game_data->components->vehicle[entity] = vehicle;

    return vehicle;
}


VehicleComponent* VehicleComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->vehicle[entity];
}


void VehicleComponent_remove(int entity) {
    VehicleComponent* vehicle = VehicleComponent_get(entity);
    if (vehicle) {
        free(vehicle);
        game_data->components->vehicle[entity] = NULL;
    }
}


WeaponComponent* WeaponComponent_add(int entity, float fire_rate, int damage, int shots,
                                     float spread, int magazine, float recoil, float range, float reload_time, 
                                     AmmoType ammo_type, Filename sound) {
    WeaponComponent* weapon = malloc(sizeof(WeaponComponent));
    weapon->cooldown = 0.0;
    weapon->fire_rate = fire_rate;
    weapon->recoil = 0.0;
    weapon->recoil_up = recoil;
    weapon->recoil_down = 3.0f * recoil;
    weapon->damage = damage;
    weapon->max_magazine = magazine;
    weapon->magazine = magazine;
    if (magazine == -1) {
        weapon->magazine = 1;
    }
    weapon->max_recoil = recoil * M_PI;
    weapon->reload_time = reload_time;
    weapon->reloading = false;
    weapon->range = range;
    weapon->sound_range = range;
    weapon->spread = spread;
    weapon->shots = shots;
    weapon->ammo_type = ammo_type;
    strcpy(weapon->sound, sound);
    weapon->automatic = false;
    weapon->penetration = 0;

    game_data->components->weapon[entity] = weapon;
    return weapon;
}


WeaponComponent* WeaponComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->weapon[entity];
}


void WeaponComponent_remove(int entity) {
    WeaponComponent* weapon = WeaponComponent_get(entity);
    if (weapon) {
        free(weapon);
        game_data->components->weapon[entity] = NULL;
    }
}


ItemComponent* ItemComponent_add(int entity, int size, int price, ButtonText name) {
    ItemComponent* item = malloc(sizeof(ItemComponent));
    item->size = size;
    for (int i = 0; i < size; i++) {
        item->attachments[i]= -1;
    }
    item->price = price;
    strcpy(item->name, name);
    item->use_time = 0.0f;
    item->type = ITEM_NONE;
    item->value = 0;

    game_data->components->item[entity] = item;
    return item;
}


ItemComponent* ItemComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->item[entity];
}


void ItemComponent_remove(int entity) {
    ItemComponent* item = ItemComponent_get(entity);
    if (item) {
        free(item);
        game_data->components->item[entity] = NULL;
    }
}


WaypointComponent* WaypointComponent_add(int entity) {
    WaypointComponent* waypoint = malloc(sizeof(WaypointComponent));
    waypoint->came_from = -1;
    waypoint->g_score = INFINITY;
    waypoint->f_score = INFINITY;
    waypoint->neighbors = List_create();
    waypoint->new_neighbors = List_create();
    waypoint->range = 16.0f;

    game_data->components->waypoint[entity] = waypoint;

    return waypoint;
}


WaypointComponent* WaypointComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->waypoint[entity];
}


void WaypointComponent_remove(int entity) {
    WaypointComponent* waypoint = WaypointComponent_get(entity);
    if (waypoint) {
        ListNode* node;
        FOREACH(node, waypoint->neighbors) {
            int n = node->value;
            WaypointComponent* neighbor = WaypointComponent_get(n);
            if (neighbor) {
                List_remove(neighbor->neighbors, entity);
            }
        }
        List_delete(waypoint->neighbors);

        FOREACH(node, waypoint->new_neighbors) {
            int n = node->value;
            WaypointComponent* neighbor = WaypointComponent_get(n);
            if (neighbor) {
                List_remove(neighbor->new_neighbors, entity);
            }
        }
        List_delete(waypoint->new_neighbors);

        free(waypoint);
        game_data->components->waypoint[entity] = NULL;
    }
}


HealthComponent* HealthComponent_add(int entity, int health, Filename dead_image, Filename decal, Filename die_sound) {
    HealthComponent* comp = malloc(sizeof(HealthComponent));
    comp->dead = health <= 0;
    comp->health = health;
    comp->max_health = health;
    strcpy(comp->dead_image, dead_image);
    strcpy(comp->decal, decal);
    strcpy(comp->die_sound, die_sound);
    comp->status.type = STATUS_NONE;
    comp->status.lifetime = 0.0f;
    comp->status.entity = NULL_ENTITY;
    comp->status.timer = 0.0f;

    game_data->components->health[entity] = comp;

    return comp;
}


HealthComponent* HealthComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->health[entity];
}


void HealthComponent_remove(int entity) {
    HealthComponent* health = HealthComponent_get(entity);
    if (health) {
        free(health);
        game_data->components->health[entity] = NULL;
    }
}


CameraComponent* CameraComponent_add(int entity, Resolution resolution, float zoom) {
    CameraComponent* camera = malloc(sizeof(CameraComponent));

    camera->resolution = resolution;
    camera->zoom_target = zoom;
    camera->zoom = camera->zoom_target * camera->resolution.h / 720.0;

    camera->matrix = (Matrix2f) { 0.0f, 0.0f, 0.0f, 0.0f };
    camera->inv_matrix = (Matrix2f) { 0.0f, 0.0f, 0.0f, 0.0f };

    camera->shake.position = zeros();
    camera->shake.velocity = rand_vector();

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


PathComponent* PathComponent_add(int entity) {
    PathComponent* path = malloc(sizeof(PathComponent));
    path->prev = -1;
    path->next = -1;
    path->width = 0.0f;

    game_data->components->path[entity] = path;
    return path;
}


PathComponent* PathComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->path[entity];
}


void PathComponent_remove(int entity) {
    PathComponent* road = PathComponent_get(entity);
    if (road) {
        if (road->prev != NULL_ENTITY) {
            PathComponent_get(road->prev)->next = road->next;
        }
        if (road->next != NULL_ENTITY) {
            PathComponent_get(road->next)->prev = road->prev;
        }
        free(road);
        game_data->components->path[entity] = NULL;
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


AmmoComponent* AmmoComponent_add(int entity, AmmoType type) {
    AmmoComponent* ammo = malloc(sizeof(AmmoComponent));
    ammo->type = type;
    switch (type) {
        case AMMO_PISTOL:
            ammo->size = 24;
            break;
        case AMMO_RIFLE:
            ammo->size = 60;
            break;
        case AMMO_SHOTGUN:
            ammo->size = 16;
            break;
        default:
            ammo->size = 0;
            break;
    }

    game_data->components->ammo[entity] = ammo;
    return ammo;
}


AmmoComponent* AmmoComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->ammo[entity];
}


void AmmoComponent_remove(int entity) {
    AmmoComponent* ammo = AmmoComponent_get(entity);
    if (ammo) {
        free(ammo);
        game_data->components->ammo[entity] = NULL;
    }
}


AnimationComponent* AnimationComponent_add(int entity, int frames) {
    AnimationComponent* anim = malloc(sizeof(AnimationComponent));
    anim->frames = frames;
    anim->current_frame = 0;
    anim->framerate = 10.0f;
    anim->timer = 0.0f;
    anim->play_once = false;
    anim->wind_factor = 0.0f;

    game_data->components->animation[entity] = anim;
    return anim;
}


AnimationComponent* AnimationComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->animation[entity];
}


void AnimationComponent_remove(int entity) {
    AnimationComponent* anim = AnimationComponent_get(entity);
    if (anim) {
        free(anim);
        game_data->components->animation[entity] = NULL;
    }
}


DoorComponent* DoorComponent_add(int entity, int price) {
    DoorComponent* door = malloc(sizeof(DoorComponent));
    door->locked = price > 0;
    door->price = price;
    door->key = KEY_NONE;

    game_data->components->door[entity] = door;
    return door;
}


DoorComponent* DoorComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->door[entity];
}


void DoorComponent_remove(int entity) {
    DoorComponent* door = DoorComponent_get(entity);
    if (door) {
        free(door);
        game_data->components->door[entity] = NULL;
    }
}


JointComponent* JointComponent_add(int entity, int parent, float min_length, float max_length, float strength) {
    JointComponent* joint = malloc(sizeof(JointComponent));
    joint->parent = parent;
    joint->min_length = min_length;
    joint->max_length = max_length;
    joint->strength = strength;
    joint->max_angle = M_PI;

    game_data->components->joint[entity] = joint;
    return joint;
}


JointComponent* JointComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->joint[entity];
}


void JointComponent_remove(int entity) {
    JointComponent* joint = JointComponent_get(entity);
    if (joint) {
        free(joint);
        game_data->components->joint[entity] = NULL;
    }
}


WidgetComponent* WidgetComponent_add(int entity, ButtonText string, WidgetType type) {
    WidgetComponent* widget = malloc(sizeof(WidgetComponent));
    widget->enabled = true;
    widget->type = type;
    widget->selected = false;
    strcpy(widget->string, string);
    widget->on_click = NULL;
    widget->on_change = NULL;
    widget->value = 0;
    widget->max_value = 0;
    widget->min_value = 0;
    widget->cyclic = false;
    widget->strings = NULL;
    
    game_data->components->widget.array[entity] = widget;
    List_append(game_data->components->widget.order, entity);
    return widget;
}


WidgetComponent* WidgetComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->widget.array[entity];
}


void WidgetComponent_remove(int entity) {
    WidgetComponent* widget = WidgetComponent_get(entity);
    if (widget) {
        free(widget);
        game_data->components->widget.array[entity] = NULL;
        List_remove(game_data->components->widget.order, entity);
    }
}


TextComponent* TextComponent_add(int entity, String string, int size, Color color) {
    TextComponent* text = malloc(sizeof(TextComponent));
    strcpy(text->source_string, string);
    text->string[0] = '\0';
    text->size = size;
    text->color = color;

    replace_actions(text->string, text->source_string);

    game_data->components->text[entity] = text;

    return text;
}


TextComponent* TextComponent_get(int entity) {
    if (entity == -1) return NULL;
    return game_data->components->text[entity];
}


void TextComponent_remove(int entity) {
    TextComponent* text = TextComponent_get(entity);
    if (text) {
        free(text);
        game_data->components->text[entity] = NULL;
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
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    if (coord->parent != -1) {
        return get_root(coord->parent);
    }
    JointComponent* joint = JointComponent_get(entity);
    if (joint && joint->parent != -1) {
        return get_root(joint->parent);
    }
    return entity;
}


void add_child(int parent, int child) {
    CoordinateComponent_get(child)->parent = parent;
    List_append(CoordinateComponent_get(parent)->children, child);
}


void remove_children(int parent) {
    CoordinateComponent* coord = CoordinateComponent_get(parent);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        CoordinateComponent_get(node->value)->parent = -1;
    }
    List_clear(coord->children);
}


void remove_parent(int child) {
    CoordinateComponent* coord = CoordinateComponent_get(child);
    if (coord->parent != NULL_ENTITY) {
        List_remove(CoordinateComponent_get(coord->parent)->children, child);
        coord->parent = NULL_ENTITY;
    }
}


void remove_prefab(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    coord->prefab[0] = '\0';
}


void destroy_entity(int entity) {
    if (entity == -1) return;

    // TODO: remove parent

    CoordinateComponent_remove(entity);
    ImageComponent_remove(entity);
    PhysicsComponent_remove(entity);
    ColliderComponent_remove(entity);
    PlayerComponent_remove(entity);
    LightComponent_remove(entity);
    EnemyComponent_remove(entity);
    ParticleComponent_remove(entity);
    VehicleComponent_remove(entity);
    WeaponComponent_remove(entity);
    ItemComponent_remove(entity);
    WaypointComponent_remove(entity);
    HealthComponent_remove(entity);
    CameraComponent_remove(entity);
    PathComponent_remove(entity);
    SoundComponent_remove(entity);
    AmmoComponent_remove(entity);
    AnimationComponent_remove(entity);
    DoorComponent_remove(entity);
    JointComponent_remove(entity);
    WidgetComponent_remove(entity);
    TextComponent_remove(entity);

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
    CoordinateComponent* coord = CoordinateComponent_get(entity);
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
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    Matrix3 transform = transform_matrix(coord->position, coord->angle, coord->scale);
    if (coord->parent != -1) {
        return matrix3_mult(get_transform(coord->parent), transform);
    }
    return transform;
}


Vector2f get_position(int entity) {
    Matrix3 transform = get_transform(entity);
    return position_from_transform(transform);
}


float get_angle(int entity) {
    Matrix3 transform = get_transform(entity);
    return angle_from_transform(transform);
}


Vector2f get_scale(int entity) {
    Matrix3 transform = get_transform(entity);
    return scale_from_transform(transform);
}


Vector2f get_position_interpolated(int entity, float delta) {
    Vector2f previous_position = CoordinateComponent_get(entity)->previous.position;
    Vector2f current_position = get_position(entity);

    float x = lerp(previous_position.x, current_position.x, delta);
    float y = lerp(previous_position.y, current_position.y, delta);

    return (Vector2f) { x, y };
}


float get_angle_interpolated(int entity, float delta) {
    float previous_angle = CoordinateComponent_get(entity)->previous.angle;
    float current_angle = get_angle(entity);

    return lerp_angle(previous_angle, current_angle, delta);
}


Vector2f get_scale_interpolated(int entity, float delta) {
    Vector2f previous_scale = CoordinateComponent_get(entity)->previous.scale;
    Vector2f current_scale = get_scale(entity);

    float x = lerp(previous_scale.x, current_scale.x, delta);
    float y = lerp(previous_scale.y, current_scale.y, delta);

    return (Vector2f) { x, y };
}


bool entity_exists(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    if (coord) {
        return true;
    }
    return false;
}


int get_parent(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    return coord->parent;
}


List* get_children(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    return coord->children;
}


Vector2f get_entities_center(List* entities) {
    Vector2f center = zeros();
    ListNode* node;
    FOREACH(node, entities) {
        int i = node->value;
        if (CoordinateComponent_get(i)->parent == -1) {
            center = sum(center, get_position(i));
        }
    }
    if (entities->size != 0) {
        center = mult(1.0f / entities->size, center);
    }

    return center;
}
