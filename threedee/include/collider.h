#pragma once

#include "component.h"
#include "camera.h"
#include "grid.h"
#include "raycast.h"


//                                                                               O
//                                                             V        B        B     
//                                                       P  E  E        A  B     S   
//                                                       L  N  H        R  U  L  T  F  V     D     E
//                                                 W  I  A  E  I  T  R  R  L  I  A  L  I     E  D  N
//                                                 A  T  Y  M  C  R  O  I  L  G  C  O  S  R  B  O  E
//                                              A  L  E  E  I  L  E  A  E  E  H  L  O  I  A  R  O  R
//                                              L  L  M  R  E  E  E  D  R  T  T  E  R  O  Y  I  R  G
//                                              L  S  S  S  S  S  S  S  S  S  S  S  S  N  S  S  S  Y
static const int COLLISION_MATRIX[18][18] = { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },     // 0 ALL
                                              { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 1 WALLS
                                              { 1, 1, 2, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 },     // 2 ITEMS
                                              { 1, 1, 0, 2, 2, 1, 2, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0 },     // 3 PLAYERS
                                              { 1, 1, 0, 2, 2, 1, 2, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0 },     // 4 ENEMIES
                                              { 1, 1, 0, 0, 0, 0, 1, 3, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },     // 5 VEHICLES
                                              { 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },     // 6 TREES
                                              { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 7 ROADS
                                              { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 8 BARRIERS
                                              { 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 },     // 9 BULLETS
                                              { 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 },     // 10 LIGHTS
                                              { 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0 },     // 11 OBSTACLES
                                              { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 12 FLOORS
                                              { 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 13 VISION
                                              { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 14 RAYS
                                              { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 15 DEBRIS
                                              { 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // 16 DOORS
                                              { 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 } };   // 17 ENERGY

float collider_width(int i);
float collider_height(int i);
float collider_radius(int i);

bool point_inside_collider(int i, Vector2f point);

void get_corners(int i, Vector2f* corners) ;

Vector2f half_width(int i);

Vector2f half_height(int i);

float axis_half_width(int i, Vector2f axis);

Vector2f overlap_rectangle_rectangle(int i, int j);

Vector2f overlap_collider_collider(int i, int j);

Vector2f overlap_rectangle_image(int i, int j);

bool collides_with(int entity, List* entities);

void update_collisions();

void draw_occupied_tiles(int camera);

void draw_colliders(int camera);
