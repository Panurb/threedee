#pragma once

#include <stdbool.h>


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

typedef struct {
    float x;
    float y;
} Vector2f;

typedef struct {
    float x;
    float y;
    float z;
} Vector3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} Vector4;

typedef struct {
    float a;
    float b;
    float c;
    float d;
} Matrix2f;

typedef struct {
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float g;
    float h;
    float i;
} Matrix3;

typedef struct {
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float g;
    float h;
    float i;
    float j;
    float k;
    float l;
    float m;
    float n;
    float o;
    float p;
} Matrix4;

typedef struct {
    int w;
    int h;
} Resolution;

typedef struct {
    int r;
    int g;
    int b;
    int a;
} Color;

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

bool close_enough(float a, float b, float epsilon);

Vector2f zeros();

Vector2f ones();

Vector2f vec(float x, float y);

float norm(Vector2f v);

float norm2(Vector2f v);

float dist(Vector2f a, Vector2f b);

Vector2f normalized(Vector2f v);

double to_degrees(double radians);

float dot(Vector2f a, Vector2f b);

float dot4(Vector4 a, Vector4 b);

float signed_angle(Vector2f a, Vector2f b);

Vector2f bisector(Vector2f a, Vector2f b);

Vector2f polar_to_cartesian(float length, float angle);

int abs_argmin(float* a, int n);

int argmax(float* a, int n);

int argmin(float* a, int n);

float mean(float* array, int size);

Vector2f perp(Vector2f v);

float sign(float x);

Vector2f sum(Vector2f v, Vector2f u);

Vector2f diff(Vector2f v, Vector2f u);

Vector2f mult(float c, Vector2f v);

Vector2f proj(Vector2f a, Vector2f b);

Vector2f lin_comb(float a, Vector2f v, float b, Vector2f u);

float randf(float min, float max);

int randi(int low, int upp);

float rand_angle();

Vector2f rand_vector();

int rand_choice(float* probs, int size);

int find(int value, int* array, int size);

int replace(int old, int new, int* array, int size);

int mini(int a, int b);

int maxi(int a, int b);

float mod(float x, float y);

float cross(Vector2f v, Vector2f u);

Vector2f rotate(Vector2f v, float angle);

float polar_angle(Vector2f v);

Matrix2f rotation_matrix(float angle);

Vector2f matrix_mult(Matrix2f m, Vector2f v);

Matrix2f transpose(Matrix2f m);

Matrix2f matrix_inverse(Matrix2f m);

Matrix3 matrix3_mult(Matrix3 m, Matrix3 n);

Matrix3 transform_matrix(Vector2f position, float angle, Vector2f scale);

Vector4 matrix4_map(Matrix4 m, Vector4 v);

Vector2f position_from_transform(Matrix3 m);

Vector2f scale_from_transform(Matrix3 m);

float angle_from_transform(Matrix3 m);

Color get_color(float r, float g, float b, float a);

void permute(int* array, int size);

float lerp(float a, float b, float t);

float lerp_angle(float a, float b, float t);

float smoothstep(float x, float mu, float nu);

int list_files_alphabetically(String path, String* files);

int binary_search_filename(String filename, String* array, int size);

bool non_zero(Vector2f v);

float clamp(float val, float min_val, float max_val);

float angle_normalized(float angle);

float angle_diff(float a, float b);

bool collides_aabb(Vector2f pos1, float w1, float h1, Vector2f pos2, float w2, float h2);

bool point_inside_rectangle(Vector2f position, float angle, float width, float height, Vector2f point);

void get_circle_points(Vector2f position, float radius, int n, Vector2f* points);

void get_ellipse_points(Vector2f position, float major, float minor, float angle, int n, Vector2f* points);

void get_rect_corners(Vector2f position, float angle, float width, float height, Vector2f* corners);

float map_to_range(int x, int min_x, int max_x, float min_y, float max_y);
