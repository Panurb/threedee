#pragma once

#include <stdbool.h>

#include "quaternion.h"


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
    float _11, _12, _13;
    float _21, _22, _23;
    float _31, _32, _33;
} Matrix3;

typedef struct {
    float _11, _12, _13, _14;
    float _21, _22, _23, _24;
    float _31, _32, _33, _34;
    float _41, _42, _43, _44;
} Matrix4;

float vec3_get(Vector3 v, int i);

void vec3_set(Vector3* v, int i, float x);

Vector2 zeros2();

Vector3 zeros3();

Vector4 zeros4();

Vector2 ones2();

Vector3 ones3();

Vector3 diag3(float value);

Vector2 vec2(float x, float y);

Vector3 vec3(float x, float y, float z);

Vector4 vec4(float x, float y, float z, float w);

float norm2(Vector2 v);

float norm3(Vector3 v);

float normsqr2(Vector2 v);

float normsqr3(Vector3 v);

float dist2(Vector2 a, Vector2 b);

Vector2 normalized2(Vector2 v);

Vector3 normalized3(Vector3 v);

Vector4 normalized4(Vector4 v);

float dot2(Vector2 a, Vector2 b);

float dot3(Vector3 a, Vector3 b);

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

Vector4 mult4(float c, Vector4 v);

Vector2 proj(Vector2 a, Vector2 b);

Vector3 proj3(Vector3 a, Vector3 b);

Vector2 lin_comb(float a, Vector2 v, float b, Vector2 u);

Vector3 cross(Vector3 v, Vector3 u);

Vector2 rotate(Vector2 v, float angle);

float polar_angle(Vector2 v);

Matrix2 rotation_matrix(float angle);

Vector2 matrix_mult(Matrix2 m, Vector2 v);

Matrix2 transpose(Matrix2 m);

Matrix4 transpose4(Matrix4 m);

Matrix2 matrix2_inverse(Matrix2 m);

Matrix3 matrix3_inverse(Matrix3 m);

Matrix4 transform_inverse(Matrix4 m);

Matrix3 matrix3_mult(Matrix3 m, Matrix3 n);

Matrix4 matrix4_mult(Matrix4 m, Matrix4 n);

Matrix3 matrix3_id(void);

Matrix4 matrix4_id();

Matrix4 transform_matrix(Vector3 position, Quaternion rotation, Vector3 scale);

Vector3 matrix3_map(Matrix3 m, Vector3 v);

Vector4 matrix4_map(Matrix4 m, Vector4 v);

Vector3 position_from_transform(Matrix4 m);

Vector3 scale_from_transform(Matrix4 m);

bool non_zero(Vector2 v);

bool non_zero3(Vector3 v);

Matrix4 perspective_projection_matrix(float fov, float aspect, float near, float far);

Matrix4 orthographic_projection_matrix(float left, float right, float bottom, float top, float near, float far);

Matrix4 look_at_matrix(Vector3 position, Vector3 forward, Vector3 up);

void vector2_print(void* ptr);

void vector3_print(void* ptr);

void matrix3_print(Matrix3 m);

void matrix4_print(Matrix4 m);

Quaternion direction_to_quaternion(Vector3 fwd, Vector3 up);

Matrix3 quaternion_to_rotation_matrix(Quaternion q);

Quaternion rotation_matrix_to_quaternion(Matrix3 m);

Quaternion axis_angle_to_quaternion(Vector3 axis, float angle);

Vector3 clamp_magnitude3(Vector3 v, float min, float max);

Vector3 clamp3(Vector3 v, Vector3 min, Vector3 max);
