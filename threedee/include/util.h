#pragma once

#include <stdbool.h>

#include "linalg.h"


#define LOGGING_LEVEL 3

#if LOGGING_LEVEL > 3
    #define LOG_DEBUG(...) printf("DEBUG: "); printf(__VA_ARGS__); printf("\n");
#else
    #define LOG_DEBUG(...)
#endif
#if LOGGING_LEVEL > 2
    #define LOG_INFO(...) printf("INFO: "); printf(__VA_ARGS__); printf("\n");
#else
    #define LOG_INFO(...)
#endif
#if LOGGING_LEVEL > 1
    #define LOG_WARNING(...) printf("WARNING: "); printf(__VA_ARGS__); printf("\n");
#else
    #define LOG_WARNING(...)
#endif
#if LOGGING_LEVEL > 0
    #define LOG_ERROR(...) printf("ERROR: "); printf(__VA_ARGS__); printf("\n");
#else
    #define LOG_ERROR(...)
#endif

#define PRINT(x) printf("%d\n", x);

#define UNUSED(x) (void)(x)

#define LENGTH(x) (int)(sizeof(x)/sizeof(x[0]))

typedef int Entity;
#define NULL_ENTITY -1

typedef struct {
    int w;
    int h;
} Resolution;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} Rect;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

typedef struct {
    Vector3 normal;
    float offset; // Distance from the origin to the plane along the normal
} Plane;

typedef struct {
    Vector3 center;
    float radius;
} Sphere;

typedef struct {
    Vector3 center;
    Vector3 half_extents;
    Quaternion rotation;
} Cuboid;

typedef union {
    Plane plane;
    Sphere sphere;
    Cuboid cuboid;
} Shape;

#define COLOR_NONE get_color(0.0f, 0.0f, 0.0f, 0.0f)
#define COLOR_WHITE get_color(1.0f, 1.0f, 1.0f, 1.0f)
#define COLOR_YELLOW get_color(1.0f, 1.0f, 0.0f, 1.0f)
#define COLOR_RED get_color(1.0f, 0.0f, 0.0f, 1.0f)
#define COLOR_GREEN get_color(0.0f, 1.0f, 0.0f, 1.0f)
#define COLOR_MAGENTA get_color(1.0f, 0.0f, 1.0f, 1.0f)
#define COLOR_BLOOD get_color(0.78f, 0.0f, 0.0f, 1.0f)
#define COLOR_ORANGE get_color(1.0f, 0.6f, 0.0f, 1.0f)
#define COLOR_BLUE get_color(0.0f, 0.0f, 1.0f, 1.0f)
#define COLOR_ENERGY get_color(0.5f, 1.0f, 0.0f, 1.0f)

#define STRING_SIZE 1024
typedef char String[STRING_SIZE];

typedef char Filename[128];

#define BUTTON_TEXT_SIZE 128
typedef char ButtonText[BUTTON_TEXT_SIZE];

void fill(int* array, int value, int size);

bool fequal(float a, float b, float epsilon);

float to_degrees(float radians);

float to_radians(float degrees);

int abs_argmin(float* a, int n);

int argmax(float* a, int n);

int argmin(float* a, int n);

float mean(float* array, int size);

float sign(float x);

float randf(float min, float max);

int randi(int low, int upp);

float rand_angle();

Vector2 rand_vector();

int rand_choice(float* probs, int size);

int find(int value, int* array, int size);

int replace(int old, int new, int* array, int size);

int mini(int a, int b);

int maxi(int a, int b);

float mod(float x, float y);

Color get_color(float r, float g, float b, float a);

void permute(int* array, int size);

float lerp(float a, float b, float t);

float lerp_angle(float a, float b, float t);

float smoothstep(float x, float mu, float nu);

int list_files_alphabetically(String path, String* files);

int binary_search_filename(String filename, String* array, int size);

float clamp(float val, float min_val, float max_val);

float angle_normalized(float angle);

float angle_diff(float a, float b);

bool collides_aabb(Vector2 pos1, float w1, float h1, Vector2 pos2, float w2, float h2);

bool point_inside_rectangle(Vector2 position, float angle, float width, float height, Vector2 point);

void get_circle_points(Vector2 position, float radius, int n, Vector2* points);

void get_ellipse_points(Vector2 position, float major, float minor, float angle, int n, Vector2* points);

void get_rect_corners(Vector2 position, float angle, float width, float height, Vector2* corners);

float map_to_range(int x, int min_x, int max_x, float min_y, float max_y);
