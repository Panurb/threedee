#define _USE_MATH_DEFINES

#include <math.h>
#define NOMINMAX

#include "linalg.h"


Vector2 zeros2() {
    return (Vector2) { 0.0f, 0.0f };
}

Vector3 zeros3() {
    return (Vector3) { 0.0f, 0.0f, 0.0f };
}

Vector2 ones2() {
    return (Vector2) { 1.0f, 1.0f };
}

Vector3 ones3() {
    return (Vector3) { 1.0f, 1.0f, 1.0f };
}

Vector2 vec2(float x, float y) {
    return (Vector2) { x, y };
}

Vector3 vec3(float x, float y, float z) {
    return (Vector3) { x, y, z };
}

float norm2(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float normsqr2(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float dist2(Vector2 a, Vector2 b) {
    float x = a.x - b.x;
    float y = a.y - b.y;
    return sqrtf(x * x + y * y);
}

Vector2 normalized2(Vector2 v) {
    float n = norm2(v);
    if (n == 0.0) {
        return v;
    }
    return (Vector2) { v.x / n, v.y / n };
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
    return normalized2(sum(normalized2(a), normalized2(b)));
}

Vector2 polar_to_cartesian(float length, float angle) {
    Vector2 v = { length * cosf(angle), length * sinf(angle) };
    return v;
}

Vector2 perp(Vector2 v) {
    return (Vector2) { -v.y, v.x };
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
    Vector2 b_norm = normalized2(b);
    return mult(dot2(a, b_norm), b_norm);
}

Vector2 lin_comb(float a, Vector2 v, float b, Vector2 u) {
    return sum(mult(a, v), mult(b, u));
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

Matrix4 matrix4_id() {
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
    return (Vector2) { norm2((Vector2) { m.a, m.d }), norm2((Vector2) { m.b, m.e }) };
}


float angle_from_transform(Matrix3 m) {
    return atan2f(m.d, m.a);
}

bool non_zero(Vector2 v) {
    return (v.x != 0.0f || v.y != 0.0f);
}
