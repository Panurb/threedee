#pragma once

#include <stdbool.h>


typedef struct {
    float x;
    float y;
} Vector2;

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
    float a, b;
    float c, d;
} Matrix2;

typedef struct {
    float a, b, c;
    float d, e, f;
    float g, h, i;
} Matrix3;

typedef struct {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
} Matrix4;

typedef struct Rotation {
    float yaw;
    float pitch;
    float roll;
} Rotation;

typedef struct Quaternion {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

Vector2 zeros2();

Vector3 zeros3();

Vector2 ones2();

Vector3 ones3();

Vector3 diag3(float value);

Vector2 vec2(float x, float y);

Vector3 vec3(float x, float y, float z);

float norm2(Vector2 v);

float normsqr2(Vector2 v);

float dist2(Vector2 a, Vector2 b);

Vector2 normalized2(Vector2 v);

float dot2(Vector2 a, Vector2 b);

float dot4(Vector4 a, Vector4 b);

float signed_angle(Vector2 a, Vector2 b);

Vector2 bisector(Vector2 a, Vector2 b);

Vector2 polar_to_cartesian(float length, float angle);

Vector2 perp(Vector2 v);

Vector2 sum(Vector2 v, Vector2 u);

Vector3 sum3(Vector3 v, Vector3 u);

Vector2 diff(Vector2 v, Vector2 u);

Vector3 diff3(Vector3 v, Vector3 u);

Vector2 mult(float c, Vector2 v);

Vector3 mult3(float c, Vector3 v);

Vector2 proj(Vector2 a, Vector2 b);

Vector2 lin_comb(float a, Vector2 v, float b, Vector2 u);

float cross(Vector2 v, Vector2 u);

Vector2 rotate(Vector2 v, float angle);

float polar_angle(Vector2 v);

Matrix2 rotation_matrix(float angle);

Vector2 matrix_mult(Matrix2 m, Vector2 v);

Matrix2 transpose(Matrix2 m);

Matrix4 transpose4(Matrix4 m);

Matrix2 matrix_inverse(Matrix2 m);

Matrix4 transform_inverse(Matrix4 m);

Matrix3 matrix3_mult(Matrix3 m, Matrix3 n);

Matrix4 matrix4_mult(Matrix4 m, Matrix4 n);

Matrix4 matrix4_id();

Matrix4 transform_matrix(Vector3 position, Rotation rotation, Vector3 scale);

Vector3 matrix3_map(Matrix3 m, Vector3 v);

Vector4 matrix4_map(Matrix4 m, Vector4 v);

Vector3 position_from_transform(Matrix4 m);

Vector3 scale_from_transform(Matrix4 m);

Vector3 rotation_from_transform(Matrix4 m);

bool non_zero(Vector2 v);

Matrix4 perspective_projection_matrix(float fov, float aspect, float near, float far);

Matrix4 orthographic_projection_matrix(float left, float right, float bottom, float top, float near, float far);

void matrix4_print(Matrix4 m);

Matrix4 look_at_matrix(Vector3 position, Vector3 forward, Vector3 up);

Vector3 direction_from_rotation(Rotation rotation);
