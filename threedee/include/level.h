#pragma once

#include <stdbool.h>


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


typedef enum RoomType {
    ROOM_EMPTY,
    ROOM_BATHROOM,
    ROOM_HALLWAY,
    ROOM_BEDROOM,
    ROOM_LIVINGROOM,
    ROOM_KITCHEN
} RoomType;


typedef struct Room {
    RoomType type;
    bool floor;
    Wall walls[4]; // front, left, back, right
} Room;


typedef struct Level {
    Room rooms[9][9];
    int width;
    int depth;
    float room_width;
    float room_depth;
    float room_height;
} Level;


Level create_level();
