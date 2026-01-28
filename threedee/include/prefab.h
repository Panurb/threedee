#pragma once


#include "component.h"


Entity create_lamp(Vector3 position);


Entity create_television(Vector3 position, float yaw);


Entity create_chair(Vector3 position, float yaw);


Entity create_table(Vector3 position, float yaw);


Entity create_book(Vector3 position, float yaw, float thickness, float height);


void create_bookcase(Vector3 position, float yaw);


void create_tree(Vector3 position);


Entity create_shrub(Vector3 position);


Entity create_blood(Vector3 position, bool hidden);


Entity create_dust_particles(Vector3 position, float width, float depth, float height);


Entity create_fire(Vector3 position, float size);


Entity create_blood_dripper(Vector3 position);
