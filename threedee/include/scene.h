#pragma once

#include "component.h"
#include "systems/particle.h"


typedef struct Scene {
    Entity camera;
    Entity screen_camera;
    Entity player;
    Entity enemy;
    Entity weather;
    ComponentData* components;
    Vector3 gravity;
    ParticleData* particles;
    struct {
        float threshold;
        float knee;
        float intensity;
        float strength;
    } bloom;
} Scene;


Scene* scene;


void create_scene();
