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

bool fequal(float a, float b, float epsilon) {
    return fabs(a - b) < epsilon;
}

float to_degrees(float radians) {
    return radians * (180.0f / M_PI);
}

float to_radians(float degrees) {
    return degrees * (M_PI / 180.0f);
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

float sign(float x) {
    if (x == 0.0) return 0.0;
    return copysignf(1.0, x);
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
    Vector2 b = diff2(a, mult(2, hh));
    Vector2 c = diff2(b, mult(2, hw));
    Vector2 d = sum(c, mult(2, hh));

    Vector2 am = diff2(point, a);
    Vector2 ab = diff2(b, a);
    Vector2 ad = diff2(d, a);

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
        points[i + 1] = sum(position, matrix_mult(rot, vec2(major * cosf(a), minor * sinf(a))));
    }
}

void get_rect_corners(Vector2 position, float angle, float width, float height, Vector2* corners) {
    Vector2 hw = polar_to_cartesian(0.5f * width, angle);
    Vector2 hh = mult(height / width, perp(hw));

    Vector2 a = sum(position, sum(hw, hh));
    Vector2 b = diff2(a, mult(2, hh));
    Vector2 c = diff2(b, mult(2, hw));
    Vector2 d = sum(c, mult(2, hh));

    corners[0] = a;
    corners[1] = b;
    corners[2] = c;
    corners[3] = d;
}


float map_to_range(int x, int min_x, int max_x, float min_y, float max_y) {
    return min_y + (max_y - min_y) * (x - min_x) / (max_x - min_x);
}
