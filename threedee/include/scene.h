#pragma once

#include "component.h"


typedef struct Scene {
    Entity camera;
    Entity screen_camera;
    Entity player;
    Entity weather;
    ComponentData* components;
} Scene;


typedef enum Wall {
    WALL_UNSET,
    WALL_NONE,
    WALL_PLAIN,
    WALL_DOOR,
    WALL_WINDOWS
} Wall;


typedef struct Coordinates {
    int x;
    int z;
} Coordinates;


typedef enum Direction {
    DIRECTION_FRONT,
    DIRECTION_RIGHT,
    DIRECTION_BACK,
    DIRECTION_LEFT,
} Direction;


typedef struct Room {
    bool floor;
    Wall walls[4]; // front, left, back, right
} Room;


typedef struct Level {
    Room rooms[9][9];
    int width;
    int depth;
    float room_width;
    float room_depth;
} Level;


Scene* scene;


void create_scene();
