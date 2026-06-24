#define _USE_MATH_DEFINES

#include <camera.h>
#include <stdio.h>
#include <stdlib.h>

#include "scene.h"
#include "systems/enemy.h"
#include "systems/player.h"
#include "util.h"
#include "component.h"
#include "level.h"


void create_scene() {
    LOG_INFO("Creating scene");

    srand(0);

    scene = malloc(sizeof(Scene));
    scene->components = ComponentData_create();
    scene->particles = ParticleData_create();
    scene->screen_camera = create_screen_camera();
    scene->player = create_player(zeros3());
    scene->bloom.threshold = 0.1f;
    scene->bloom.knee = 0.05f;
    scene->bloom.intensity = 0.5f;
    scene->bloom.strength = 0.5f;

    TransformComponent* trans = get_component(scene->player, COMPONENT_TRANSFORM);
    scene->camera = trans->children->head->value;
    // scene->camera = create_overhead_camera();
    scene->gravity = vec3(0.0f, -9.81f, 0.0f);

    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, -0.51f, 0.0f),
        .scale = vec3(100.0f, 1.0f, 100.0f)
    });
    MeshComponent_add(i, (MeshParameters) {
        .mesh_filename = "cube",
        .texture_filename = "gravel",
        .material_filename = "concrete"
    });

    Level level = create_level();

    while (true) {
        int x = randi(0, level.width - 1);
        int z = randi(0, level.depth - 1);

        if (x == level.width / 2 && z == level.depth / 2) {
            continue; // Skip the starting room
        }

        if (level.rooms[z][x].floor) {
            create_enemy(vec3(
                (x - level.width / 2) * level.room_width,
                0.0f,
                (z - level.depth / 2) * level.room_depth
            ), randf(0.0f, 360.0f));
            break;
        }
    }

    scene->weather = create_entity();
    WeatherComponent_add(scene->weather, (WeatherParameters) {
        .fog_color = COLOR_SKY,
        .fog_start = 10.0f,
        .fog_end = 50.0f,
        .ambient_light = 0.05f,
    });

    LOG_INFO("Scene created");
}
