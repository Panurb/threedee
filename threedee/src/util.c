#define _USE_MATH_DEFINES

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#define NOMINMAX
#ifndef __EMSCRIPTEN__
    #include <windows.h>
#else 
    #include <stdio.h>
    #include <dirent.h>
#endif

#include "util.h"


void fill(int* array, int value, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = value;
    }
}

bool close_enough(float a, float b, float epsilon) {
    return fabs(a - b) < epsilon;
}

Vector2 zeros2() {
    return (Vector2) { 0.0f, 0.0f };
}

Vector3 zeros3() {
    return (Vector3) { 0.0f, 0.0f, 0.0f };
}

Vector2 ones() {
    return (Vector2) { 1.0f, 1.0f };
}

Vector3 ones3() {
    return (Vector3) { 1.0f, 1.0f, 1.0f };
}

Vector2 vec(float x, float y) {
    return (Vector2) { x, y };
}

Vector3 vec3(float x, float y, float z) {
    return (Vector3) { x, y, z };
}

float norm(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float norm2(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float dist(Vector2 a, Vector2 b) {
    float x = a.x - b.x;
    float y = a.y - b.y;
    return sqrtf(x * x + y * y);
}

Vector2 normalized(Vector2 v) {
    float n = norm(v);
    if (n == 0.0) {
        return v;
    }
    return (Vector2) { v.x / n, v.y / n };
}

double to_degrees(double radians) {
    return radians * (180.0 / M_PI);
}

float dot2(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

float dot4(Vector4 a, Vector4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float signed_angle(Vector2 a, Vector2 b) {
    // https://stackoverflow.com/questions/2150050/finding-signed-angle-between-vectors
    return atan2f(a.x * b.y - a.y * b.x, a.x * b.x + a.y * b.y);
}

Vector2 bisector(Vector2 a, Vector2 b) {
    return normalized(sum(normalized(a), normalized(b)));
}

Vector2 polar_to_cartesian(float length, float angle) {
    Vector2 v = { length * cosf(angle), length * sinf(angle) };
    return v;
}

int abs_argmin(float* a, int n) {
    int i = 0;

    for (int j = 1; j < n; j++) {
        if (fabs(a[j]) < fabs(a[i])) {
            i = j;
        }
    }

    return i;
}

int argmax(float* a, int n) {
    int i = 0;

    for (int j = 1; j < n; j++) {
        if (a[j] > a[i]) {
            i = j;
        }
    }

    return i;
}

int argmin(float* a, int n) {
    int i = 0;

    for (int j = 1; j < n; j++) {
        if (a[j] < a[i]) {
            i = j;
        }
    }

    return i;
}

float mean(float* array, int size) {
    float tot = 0.0;
    for (int i = 0; i < size; i++) {
        tot += array[i];
    }
    return tot / size;
}

Vector2 perp(Vector2 v) {
    return (Vector2) { -v.y, v.x };
}

float sign(float x) {
    if (x == 0.0) return 0.0;
    return copysignf(1.0, x);
}

Vector2 sum(Vector2 v, Vector2 u) {
    return (Vector2) { v.x + u.x, v.y + u.y };
}

Vector2 diff(Vector2 v, Vector2 u) {
    return (Vector2) { v.x - u.x, v.y - u.y };
}

Vector2 mult(float c, Vector2 v) {
    return (Vector2) { c * v.x, c * v.y };
}

Vector2 proj(Vector2 a, Vector2 b) {
    Vector2 b_norm = normalized(b);
    return mult(dot2(a, b_norm), b_norm);
}

Vector2 lin_comb(float a, Vector2 v, float b, Vector2 u) {
    return sum(mult(a, v), mult(b, u));
}

float randf(float low, float upp) {
    float scale = rand() / (float) RAND_MAX;
    return low + scale * (upp - low);
}

int randi(int low, int upp) {
    return rand() % (upp + 1 - low) + low;
}

float rand_angle() {
    return randf(0.0, 2 * M_PI);
}

Vector2 rand_vector() {
    return polar_to_cartesian(1.0, rand_angle());
}

int rand_choice(float* probs, int size) {
    float total = 0.0f;
    for (int i = 0; i < size; i++) {
        total += probs[i];
    }

    float x = randf(0.0f, total);
    float cum_total = 0.0f;
    for (int i = 0; i < size; i++) {
        cum_total += probs[i];
        if (cum_total >= x) {
            return i;
        }
    }
    return size - 1;
}

int find(int value, int* array, int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] == value) {
            return i;
        }
    }

    return -1;
}

int replace(int old, int new, int* array, int size) {
    int i = find(old, array, size);
    if (i != -1) {
        array[i] = new;
    }
    return i;
}

int mini(int a, int b) {
    return (a < b) ? a : b;
}

int maxi(int a, int b) {
    return (a > b) ? a : b;
}

float mod(float x, float y) {
    return fmodf(fmodf(x, y) + y, y);
}

float cross(Vector2 v, Vector2 u) {
    return v.x * u.y - v.y * u.x;
}

Vector2 rotate(Vector2 v, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return (Vector2) { v.x * c - v.y * s, v.x * s + v.y * c };
}

float polar_angle(Vector2 v) {
    return atan2f(v.y, v.x);
}

Matrix2 rotation_matrix(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return (Matrix2) { c, -s, s, c };
}

Vector2 matrix_mult(Matrix2 m, Vector2 v) {
    return (Vector2) { m.a * v.x + m.b * v.y, m.c * v.x + m.d * v.y };
}

Matrix2 transpose(Matrix2 m) {
    return (Matrix2) { m.a, m.c, m.b, m.d };
}

Matrix2 matrix_inverse(Matrix2 m) {
    float det = m.a * m.d - m.b * m.c;
    return (Matrix2) { m.d / det, -m.b / det, -m.c / det, m.a / det };
}

Matrix3 matrix3_mult(Matrix3 m, Matrix3 n) {
    Matrix3 mn;
    mn.a = m.a * n.a + m.b * n.d + m.c * n.g;
    mn.b = m.a * n.b + m.b * n.e + m.c * n.h;
    mn.c = m.a * n.c + m.b * n.f + m.c * n.i;
    mn.d = m.d * n.a + m.e * n.d + m.f * n.g;
    mn.e = m.d * n.b + m.e * n.e + m.f * n.h;
    mn.f = m.d * n.c + m.e * n.f + m.f * n.i;
    mn.g = m.g * n.a + m.h * n.d + m.i * n.g;
    mn.h = m.g * n.b + m.h * n.e + m.i * n.h;
    mn.i = m.g * n.c + m.h * n.f + m.i * n.i;

    return mn;
}

Matrix4 identity4() {
    return (Matrix4) {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix4 transform_matrix(Vector3 position, Rotation rotation, Vector3 scale) {
    Matrix4 m;
    float c = cosf(rotation.x);
    float s = sinf(rotation.x);
    float c2 = cosf(rotation.y);
    float s2 = sinf(rotation.y);
    float c3 = cosf(rotation.z);
    float s3 = sinf(rotation.z);

    m.a = c2 * c3 * scale.x;
    m.b = -c2 * s3 * scale.x;
    m.c = s2 * scale.x;
    m.d = position.x;

    m.e = (s * s2 * c3 + c * s3) * scale.y;
    m.f = (-s * s2 * s3 + c * c3) * scale.y;
    m.g = -s * c2 * scale.y;
    m.h = position.y;

    m.i = (-c * s2 * c3 + s * s3) * scale.z;
    m.j = (c * s2 * s3 + s * c3) * scale.z;
    m.k = c * c2 * scale.z;
    m.l = position.z;

    m.m = 0.0f; // Homogeneous coordinate
    m.n = 0.0f; // Homogeneous coordinate
    m.o = 0.0f; // Homogeneous coordinate
    m.p = 1.0f; // Homogeneous coordinate

    return m;
}

Vector4 matrix4_map(Matrix4 m, Vector4 v) {
    Vector4 result;
    result.x = m.a * v.x + m.b * v.y + m.c * v.z + m.d * v.w;
    result.y = m.e * v.x + m.f * v.y + m.g * v.z + m.h * v.w;
    result.z = m.i * v.x + m.j * v.y + m.k * v.z + m.l * v.w;
    result.w = m.m * v.x + m.n * v.y + m.o * v.z + m.p * v.w;
    return result;
}

Vector2 position_from_transform(Matrix3 m) {
    return (Vector2) { m.c, m.f };
}


Vector2 scale_from_transform(Matrix3 m) {
    return (Vector2) { norm((Vector2) { m.a, m.d }), norm((Vector2) { m.b, m.e }) };
}


float angle_from_transform(Matrix3 m) {
    return atan2f(m.d, m.a);
}


Color get_color(float r, float g, float b, float a) {
    Color color;
    color.r = (int) (r * 255);
    color.g = (int) (g * 255);
    color.b = (int) (b * 255);
    color.a = (int) (a * 255);
    return color;
}

void permute(int* array, int size) {
    for (int i = 0; i < size; i++) {
        // Warning: skewed distribution
        int j = rand() % (i + 1);
        int t = array[i];
        array[i] = array[j];
        array[j] = t;
    }
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float lerp_angle(float a, float b, float t) {
    float d = angle_diff(b, a);
    return angle_normalized(a + t * d);
}

float smoothstep(float x, float mu, float nu) {
    // https://wernerantweiler.ca/blog.php?item=2018-11-03
    return powf(1.0 + powf(x * (1.0 - mu) / (mu * (1.0 - x)), -nu), -1.0);
}

int list_files_alphabetically(String path, String* files) {
    int files_size = 0;

    #ifndef __EMSCRIPTEN__
        WIN32_FIND_DATA file;
        HANDLE handle = FindFirstFile(path, &file);

        if (handle == INVALID_HANDLE_VALUE) {
            LOG_WARNING("Path not found: %s", path);
            return 0;
        }

        do {
            if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0) {
                continue;
            }
            char* dot = strchr(file.cFileName, '.');
            if (dot) {
                *dot = '\0';
            }
            strcpy(files[files_size], file.cFileName);
            files_size++;
        } while (FindNextFile(handle, &file));

        qsort(files, files_size, sizeof(String), strcmp);
    #else
        char* last_slash = strrchr(path, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
        DIR* dir = opendir(path);
        if (dir == NULL) {
            LOG_WARNING("Path not found: %s", path);
            return 0;
        }
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            char* dot = strchr(entry->d_name, '.');
            if (dot) {
                *dot = '\0';
            }
            LOG_INFO("Found file: %s", entry->d_name);
            strcpy(files[files_size], entry->d_name);
            files_size++;
        }
        closedir(dir);
    #endif

    return files_size;
}

int binary_search_filename(String filename, String* array, int size) {
    int l = 0;
    int r = size - 1;

    while (l <= r) {
        int m = floor((l + r) / 2);

        if (strcmp(array[m], filename) < 0) {
            l = m + 1;
        } else if (strcmp(array[m], filename) > 0) {
            r = m - 1;
        } else {
            return m;
        }
    }

    return -1;
}

bool non_zero(Vector2 v) {
    return (v.x != 0.0f || v.y != 0.0f);
}

float clamp(float val, float min_val, float max_val) {
    return fmaxf(fminf(val, max_val), min_val);
}

float angle_normalized(float angle) {
    // Normalize angle to [-pi, pi]
    return mod(angle + M_PI, 2.0f * M_PI) - M_PI;
}

float angle_diff(float a, float b) {
    return angle_normalized(a - b);
}

bool collides_aabb(Vector2 pos1, float w1, float h1, Vector2 pos2, float w2, float h2) {
    if (pos1.x < pos2.x + w2 && pos1.x + w1 > pos2.x && pos1.y < pos2.y + h2 && pos1.y + h1 > pos2.y) {
        return true;
    }
    return false;
}

bool point_inside_rectangle(Vector2 position, float angle, float width, float height, Vector2 point) {
    Vector2 hw = polar_to_cartesian(0.5f * width, angle);
    Vector2 hh = mult(height / width, perp(hw));

    Vector2 a = sum(position, sum(hw, hh));
    Vector2 b = diff(a, mult(2, hh));
    Vector2 c = diff(b, mult(2, hw));
    Vector2 d = sum(c, mult(2, hh));

    Vector2 am = diff(point, a);
    Vector2 ab = diff(b, a);
    Vector2 ad = diff(d, a);

    if (0.0f < dot2(am, ab) && dot2(am, ab) < dot2(ab, ab) && 0.0f < dot2(am, ad) && dot2(am, ad) < dot2(ad, ad)) {
        return true;
    }
    return false;
}

void get_circle_points(Vector2 position, float radius, int n, Vector2* points) {
    points[0] = position;
    for (int i = 0; i < n - 1; i++) {
        float angle = 2.0f * M_PI * i / (n - 1);
        points[i + 1] = sum(position, polar_to_cartesian(radius, angle));
    }
}

void get_ellipse_points(Vector2 position, float major, float minor, float angle, int n, Vector2* points) {
    points[0] = position;
    Matrix2 rot = rotation_matrix(angle);

    for (int i = 0; i < n - 1; i++) {
        float a = 2.0f * M_PI * i / (n - 1);
        points[i + 1] = sum(position, matrix_mult(rot, vec(major * cosf(a), minor * sinf(a))));
    }
}

void get_rect_corners(Vector2 position, float angle, float width, float height, Vector2* corners) {
    Vector2 hw = polar_to_cartesian(0.5f * width, angle);
    Vector2 hh = mult(height / width, perp(hw));

    Vector2 a = sum(position, sum(hw, hh));
    Vector2 b = diff(a, mult(2, hh));
    Vector2 c = diff(b, mult(2, hw));
    Vector2 d = sum(c, mult(2, hh));

    corners[0] = a;
    corners[1] = b;
    corners[2] = c;
    corners[3] = d;
}


float map_to_range(int x, int min_x, int max_x, float min_y, float max_y) {
    return min_y + (max_y - min_y) * (x - min_x) / (max_x - min_x);
}
