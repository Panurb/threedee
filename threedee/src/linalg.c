#define _USE_MATH_DEFINES

#include <math.h>
#define NOMINMAX

#include "linalg.h"

#include <stdio.h>
#include <util.h>


float vec3_get(Vector3 v, int i) {
    switch (i) {
        case 0: return v.x;
        case 1: return v.y;
        case 2: return v.z;
        default:
            LOG_ERROR("Invalid index %d for Vector3", i);
            return 0.0f;
    }
}

void vec3_set(Vector3* v, int i, float x) {
    switch (i) {
        case 0: v->x = x; break;
        case 1: v->y = x; break;
        case 2: v->z = x; break;
        default:
            LOG_ERROR("Invalid index %d for Vector3", i);
            break;
    }
}

Vector2 zeros2() {
    return (Vector2) { 0.0f, 0.0f };
}

Vector3 zeros3() {
    return (Vector3) { 0.0f, 0.0f, 0.0f };
}

Vector4 zeros4() {
    return (Vector4) { 0.0f, 0.0f, 0.0f, 0.0f };
}

Vector2 ones2() {
    return (Vector2) { 1.0f, 1.0f };
}

Vector3 ones3() {
    return (Vector3) { 1.0f, 1.0f, 1.0f };
}

Vector3 diag3(float value) {
    return (Vector3) { value, value, value };
}

Vector2 vec2(float x, float y) {
    return (Vector2) { x, y };
}

Vector3 vec3(float x, float y, float z) {
    return (Vector3) { x, y, z };
}

Vector4 vec4(float x, float y, float z, float w) {
    return (Vector4) { x, y, z, w };
}

float norm2(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float norm3(Vector3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float normsqr2(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float normsqr3(Vector3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
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

Vector3 normalized3(Vector3 v) {
    float n = norm3(v);
    if (n == 0.0f) {
        return v;
    }
    return (Vector3) { v.x / n, v.y / n, v.z / n };
}

Vector4 normalized4(Vector4 v) {
    float n = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (n == 0.0f) {
        return v;
    }
    return (Vector4) { v.x / n, v.y / n, v.z / n, v.w / n };
}

float dot2(Vector2 a, Vector2 b) {
    return a.x * b.x + a.y * b.y;
}

float dot3(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
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

Vector3 sum3(Vector3 v, Vector3 u) {
    return (Vector3) { v.x + u.x, v.y + u.y, v.z + u.z };
}

Vector2 diff2(Vector2 v, Vector2 u) {
    return (Vector2) { v.x - u.x, v.y - u.y };
}

Vector3 diff3(Vector3 v, Vector3 u) {
    return (Vector3) { v.x - u.x, v.y - u.y, v.z - u.z };
}

Vector2 mult(float c, Vector2 v) {
    return (Vector2) { c * v.x, c * v.y };
}

Vector3 mult3(float c, Vector3 v) {
    return (Vector3) { c * v.x, c * v.y, c * v.z };
}

Vector4 mult4(float c, Vector4 v) {
    return (Vector4) { c * v.x, c * v.y, c * v.z, c * v.w };
}

Vector3 neg3(Vector3 v) {
    return (Vector3) { -v.x, -v.y, -v.z };
}

Vector3 div3(float c, Vector3 v) {
    if (c == 0.0f) {
        LOG_ERROR("Division by zero in vector division");
        return v;
    }
    return (Vector3) { v.x / c, v.y / c, v.z / c };
}

Vector2 proj(Vector2 a, Vector2 b) {
    Vector2 b_norm = normalized2(b);
    return mult(dot2(a, b_norm), b_norm);
}

Vector3 proj3(Vector3 a, Vector3 b) {
    Vector3 b_norm = normalized3(b);
    return mult3(dot3(a, b_norm), b_norm);
}

Vector2 lin_comb(float a, Vector2 v, float b, Vector2 u) {
    return sum(mult(a, v), mult(b, u));
}

Vector3 cross(Vector3 v, Vector3 u) {
    return (Vector3) {
        v.y * u.z - v.z * u.y,
        v.z * u.x - v.x * u.z,
        v.x * u.y - v.y * u.x
    };
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

Matrix3 transpose3(Matrix3 m) {
    return (Matrix3) {
        m._11, m._21, m._31,
        m._12, m._22, m._32,
        m._13, m._23, m._33
    };
}

Matrix4 transpose4(Matrix4 m) {
    return (Matrix4) {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44
    };
}

float matrix2_determinant(Matrix2 m) {
    return m.a * m.d - m.b * m.c;
}

float matrix3_determinant(Matrix3 m) {
    return m._11 * (m._22 * m._33 - m._23 * m._32) -
           m._12 * (m._21 * m._33 - m._23 * m._31) +
           m._13 * (m._21 * m._32 - m._22 * m._31);
}

Matrix2 matrix2_inverse(Matrix2 m) {
    float det = matrix2_determinant(m);
    return (Matrix2) { m.d / det, -m.b / det, -m.c / det, m.a / det };
}

Matrix3 matrix3_inverse(Matrix3 m) {
    float det = matrix3_determinant(m);

    if (fabsf(det) < 1e-6f) {
        LOG_ERROR("Matrix3 is singular, cannot compute inverse");
        return matrix3_id();
    }

    float inv_det = 1.0f / det;

    Matrix3 inv;
    inv._11 =  (m._22 * m._33 - m._23 * m._32) * inv_det;
    inv._12 = -(m._12 * m._33 - m._13 * m._32) * inv_det;
    inv._13 =  (m._12 * m._23 - m._13 * m._22) * inv_det;
    inv._21 = -(m._21 * m._33 - m._23 * m._31) * inv_det;
    inv._22 =  (m._11 * m._33 - m._13 * m._31) * inv_det;
    inv._23 = -(m._11 * m._23 - m._13 * m._21) * inv_det;
    inv._31 =  (m._21 * m._32 - m._22 * m._31) * inv_det;
    inv._32 = -(m._11 * m._32 - m._12 * m._31) * inv_det;
    inv._33 =  (m._11 * m._22 - m._12 * m._21) * inv_det;

    return inv;
}

Matrix4 transform_inverse(Matrix4 m) {
    Matrix3 r = {
        m._11, m._21, m._31,
        m._12, m._22, m._32,
        m._13, m._23, m._33
    };

    Vector3 d = {
        m._14,
        m._24,
        m._34
    };

    Vector3 rd = matrix3_map(r, d);

    Matrix4 m_inv = {
        r._11, r._12, r._13, -rd.x,
        r._21, r._22, r._23, -rd.y,
        r._31, r._32, r._33, -rd.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    return m_inv;
}

Matrix3 matrix3_mult(Matrix3 m, Matrix3 n) {
    Matrix3 mn;
    mn._11 = m._11 * n._11 + m._12 * n._21 + m._13 * n._31;
    mn._12 = m._11 * n._12 + m._12 * n._22 + m._13 * n._32;
    mn._13 = m._11 * n._13 + m._12 * n._23 + m._13 * n._33;
    mn._21 = m._21 * n._11 + m._22 * n._21 + m._23 * n._31;
    mn._22 = m._21 * n._12 + m._22 * n._22 + m._23 * n._32;
    mn._23 = m._21 * n._13 + m._22 * n._23 + m._23 * n._33;
    mn._31 = m._31 * n._11 + m._32 * n._21 + m._33 * n._31;
    mn._32 = m._31 * n._12 + m._32 * n._22 + m._33 * n._32;
    mn._33 = m._31 * n._13 + m._32 * n._23 + m._33 * n._33;

    return mn;
}

Matrix4 matrix4_mult(Matrix4 m, Matrix4 n) {
    // Do not assume homogenous matrices
    Matrix4 mn;
    mn._11 = m._11 * n._11 + m._12 * n._21 + m._13 * n._31 + m._14 * n._41;
    mn._12 = m._11 * n._12 + m._12 * n._22 + m._13 * n._32 + m._14 * n._42;
    mn._13 = m._11 * n._13 + m._12 * n._23 + m._13 * n._33 + m._14 * n._43;
    mn._14 = m._11 * n._14 + m._12 * n._24 + m._13 * n._34 + m._14 * n._44;
    mn._21 = m._21 * n._11 + m._22 * n._21 + m._23 * n._31 + m._24 * n._41;
    mn._22 = m._21 * n._12 + m._22 * n._22 + m._23 * n._32 + m._24 * n._42;
    mn._23 = m._21 * n._13 + m._22 * n._23 + m._23 * n._33 + m._24 * n._43;
    mn._24 = m._21 * n._14 + m._22 * n._24 + m._23 * n._34 + m._24 * n._44;
    mn._31 = m._31 * n._11 + m._32 * n._21 + m._33 * n._31 + m._34 * n._41;
    mn._32 = m._31 * n._12 + m._32 * n._22 + m._33 * n._32 + m._34 * n._42;
    mn._33 = m._31 * n._13 + m._32 * n._23 + m._33 * n._33 + m._34 * n._43;
    mn._34 = m._31 * n._14 + m._32 * n._24 + m._33 * n._34 + m._34 * n._44;
    mn._41 = m._41 * n._11 + m._42 * n._21 + m._43 * n._31 + m._44 * n._41;
    mn._42 = m._41 * n._12 + m._42 * n._22 + m._43 * n._32 + m._44 * n._42;
    mn._43 = m._41 * n._13 + m._42 * n._23 + m._43 * n._33 + m._44 * n._43;
    mn._44 = m._41 * n._14 + m._42 * n._24 + m._43 * n._34 + m._44 * n._44;
    return mn;
}

Matrix3 matrix3_id(void) {
    return (Matrix3) {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
}

Matrix4 matrix4_id() {
    return (Matrix4) {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix4 transform_matrix(Vector3 position, Quaternion rotation, Vector3 scale) {
    Matrix3 r = quaternion_to_rotation_matrix(rotation);
    // Quaternion q = rotation_matrix_to_quaternion(r);
    //
    // if (!quaternion_equals(rotation, q)) {
    //     LOG_ERROR("Quaternion and rotation matrix do not match!");
    //     LOG_ERROR("Quaternion: %f, %f, %f, %f", rotation.x, rotation.y, rotation.z, rotation.w);
    //     LOG_ERROR("Quaternion2: %f, %f, %f, %f", q.x, q.y, q.z, q.w);
    // }

    Matrix4 rot = {
        r._11, r._12, r._13, 0.0f,
        r._21, r._22, r._23, 0.0f,
        r._31, r._32, r._33, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Scale matrix
    Matrix4 scl = {
        scale.x, 0.0f,    0.0f,    0.0f,
        0.0f,    scale.y, 0.0f,    0.0f,
        0.0f,    0.0f,    scale.z, 0.0f,
        0.0f,    0.0f,    0.0f,    1.0f
    };

    // Translation matrix
    Matrix4 trans = {
        1.0f, 0.0f, 0.0f, position.x,
        0.0f, 1.0f, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Combine: T * R * S
    return matrix4_mult(trans, matrix4_mult(rot, scl));
}

Vector3 matrix3_map(Matrix3 m, Vector3 v) {
    Vector3 result;
    result.x = m._11 * v.x + m._12 * v.y + m._13 * v.z;
    result.y = m._21 * v.x + m._22 * v.y + m._23 * v.z;
    result.z = m._31 * v.x + m._32 * v.y + m._33 * v.z;
    return result;
}

Vector4 matrix4_map(Matrix4 m, Vector4 v) {
    Vector4 result;
    result.x = m._11 * v.x + m._12 * v.y + m._13 * v.z + m._14 * v.w;
    result.y = m._21 * v.x + m._22 * v.y + m._23 * v.z + m._24 * v.w;
    result.z = m._31 * v.x + m._32 * v.y + m._33 * v.z + m._34 * v.w;
    result.w = m._41 * v.x + m._42 * v.y + m._43 * v.z + m._44 * v.w;
    return result;
}

Vector3 position_from_transform(Matrix4 m) {
    return (Vector3) { m._14, m._24, m._34 };
}


Vector3 scale_from_transform(Matrix4 m) {
    return (Vector3) {
        sqrtf(m._11 * m._11 + m._21 * m._21 + m._31 * m._31),
        sqrtf(m._12 * m._12 + m._22 * m._22 + m._32 * m._32),
        sqrtf(m._13 * m._13 + m._23 * m._23 + m._33 * m._33)
    };
}

Quaternion rotation_from_transform(Matrix4 m) {
    Vector3 scale = scale_from_transform(m);
    Matrix3 r = {
        m._11 / scale.x, m._12 / scale.y, m._13 / scale.z,
        m._21 / scale.x, m._22 / scale.y, m._23 / scale.z,
        m._31 / scale.x, m._32 / scale.y, m._33 / scale.z
    };
    return rotation_matrix_to_quaternion(r);
}


Vector3 quaternion_forward(Quaternion q) {
    return (Vector3) {
        -2.0f * (q.x * q.z + q.w * q.y),
        -2.0f * (q.y * q.z - q.w * q.x),
        -(1.0f - 2.0f * (q.x * q.x + q.y * q.y))
    };
}

bool non_zero(Vector2 v) {
    return (v.x != 0.0f || v.y != 0.0f);
}

bool non_zero3(Vector3 v) {
    return (v.x != 0.0f || v.y != 0.0f || v.z != 0.0f);
}

Matrix4 perspective_projection_matrix(float fov, float aspect_ratio, float near, float far) {
    // Depth is mapped to [0, 1] range as expected by Vulkan
    float f = 1.0f / tanf(fov / 2.0f);
    Matrix4 m = matrix4_id();
    m._11 = f / aspect_ratio;
    m._22 = f;
    m._33 = far / (near - far);
    m._34 = (far * near) / (near - far);
    m._43 = -1.0f;
    m._44 = 0.0f;
    return m;
}

Matrix4 orthographic_projection_matrix(float left, float right, float bottom, float top, float near, float far) {
    Matrix4 m = matrix4_id();
    m._11 = 2.0f / (right - left);
    m._22 = 2.0f / (top - bottom);
    m._33 = 1.0f / (near - far);
    m._41 = -(right + left) / (right - left);
    m._42 = -(top + bottom) / (top - bottom);
    m._43 = near / (near - far);
    return m;
}

Matrix4 look_at_matrix(Vector3 position, Vector3 target, Vector3 up) {
    Vector3 forward = normalized3(diff3(target, position));
    Vector3 right = normalized3(cross(forward, up));
    up = normalized3(cross(right, forward));
    Matrix4 m = {
        right.x, up.x, -forward.x, 0.0f,
        right.y, up.y, -forward.y, 0.0f,
        right.z, up.z, -forward.z, 0.0f,
        -dot3(right, position), -dot3(up, position), dot3(forward, position), 1.0f
    };
    return transpose4(m);
}

void vector2_print(void* ptr) {
    Vector2* v = ptr;
    printf("Vector2(%.2f, %.2f)\n", v->x, v->y);
}

void vector3_print(void* ptr) {
    Vector3* v = ptr;
    printf("Vector3(%.2f, %.2f, %.2f)\n", v->x, v->y, v->z);
}

void matrix3_print(Matrix3 m) {
    printf("[[%.2f, %.2f, %.2f]\n", m._11, m._12, m._13);
    printf("[%.2f, %.2f, %.2f]\n", m._21, m._22, m._23);
    printf("[%.2f, %.2f, %.2f]]\n", m._31, m._32, m._33);
}

void matrix4_print(Matrix4 m) {
    printf("[[%.2f, %.2f, %.2f, %.2f]\n", m._11, m._12, m._13, m._14);
    printf("[%.2f, %.2f, %.2f, %.2f]\n", m._21, m._22, m._23, m._24);
    printf("[%.2f, %.2f, %.2f, %.2f]\n", m._31, m._32, m._33, m._34);
    printf("[%.2f, %.2f, %.2f, %.2f]]\n", m._41, m._42, m._43, m._44);
}


Quaternion direction_to_quaternion(Vector3 fwd, Vector3 up) {
    LOG_INFO("Forward: %f, %f, %f", fwd.x, fwd.y, fwd.z);
    Vector3 right = cross(up, fwd);
    float right_norm = norm3(right);
    if (right_norm < 1e-6f) {
        if (dot3(up, fwd) > 0.0f) {
            // Up and forward are aligned, return identity quaternion
            return quaternion_id();
        } else {
            // Up and forward are opposite, return 180-degree rotation around right axis
            return (Quaternion) { 0.0f, 1.0f, 0.0f, 0.0f };
        }
    }

    Vector3 new_up = cross(fwd, right);

    Matrix3 rot_matrix = {
        right.x, new_up.x, fwd.x,
        right.y, new_up.y, fwd.y,
        right.z, new_up.z, fwd.z,
    };

    Quaternion q = rotation_matrix_to_quaternion(rot_matrix);
    q = quaternion_normalize(q);

    return q;
}


Matrix3 quaternion_to_rotation_matrix(Quaternion q) {
    q = quaternion_normalize(q);

    // Convert quaternion to rotation matrix
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;

    Matrix3 rot = {
        1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y),
        2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x),
        2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y),
    };

    return rot;
}


Quaternion rotation_matrix_to_quaternion(Matrix3 m) {
    // Convert rotation matrix to quaternion
    Quaternion q;
    float trace = m._11 + m._22 + m._33;

    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f; // S=4*q.w
        q.w = 0.25f * s;
        q.x = (m._32 - m._23) / s;
        q.y = (m._13 - m._31) / s;
        q.z = (m._21 - m._12) / s;
    } else if ((m._11 > m._22) && (m._11 > m._33)) {
        float s = sqrtf(1.0f + m._11 - m._22 - m._33) * 2.0f; // S=4*q.x
        q.w = (m._32 - m._23) / s;
        q.x = 0.25f * s;
        q.y = (m._12 + m._21) / s;
        q.z = (m._13 + m._31) / s;
    } else if (m._22 > m._33) {
        float s = sqrtf(1.0f + m._22 - m._11 - m._33) * 2.0f; // S=4*q.y
        q.w = (m._13 - m._31) / s;
        q.x = (m._12 + m._21) / s;
        q.y = 0.25f * s;
        q.z = (m._23 + m._32) / s;
    } else {
        float s = sqrtf(1.0f + m._33 - m._11 - m._22) * 2.0f; // S=4*q.z
        q.w = (m._21 - m._12) / s;
        q.x = (m._13 + m._31) / s;
        q.y = (m._23 + m._32) / s;
        q.z = 0.25f * s;
    }

    return q;
}

Quaternion axis_angle_to_quaternion(Vector3 axis, float angle) {
    Vector3 norm_axis = normalized3(axis);
    float half_angle = angle / 2.0f;
    float s = sinf(half_angle);
    return (Quaternion) { norm_axis.x * s, norm_axis.y * s, norm_axis.z * s, cosf(half_angle) };
}

Vector3 clamp_magnitude3(Vector3 v, float min, float max) {
    float n = norm3(v);
    if (n == 0.0f) {
        return v;
    }
    if (n < min) {
        return mult3(min / n, v);
    }
    if (n > max) {
        return mult3(max / n, v);
    }
    return v;
}


Vector3 clamp3(Vector3 v, Vector3 min, Vector3 max) {
    return (Vector3) {
        clamp(v.x, min.x, max.x),
        clamp(v.y, min.y, max.y),
        clamp(v.z, min.z, max.z)
    };
}


Matrix3 matrix3_abs(Matrix3 m) {
    return (Matrix3) {
        fabsf(m._11), fabsf(m._12), fabsf(m._13),
        fabsf(m._21), fabsf(m._22), fabsf(m._23),
        fabsf(m._31), fabsf(m._32), fabsf(m._33)
    };
}

Matrix3 matrix3_add_scalar(Matrix3 m, float c) {
    return (Matrix3) {
        m._11 + c, m._12 + c, m._13 + c,
        m._21 + c, m._22 + c, m._23 + c,
        m._31 + c, m._32 + c, m._33 + c
    };
}

Vector3 matrix3_row(Matrix3 m, int i) {
    switch (i) {
        case 0: return (Vector3) { m._11, m._12, m._13 };
        case 1: return (Vector3) { m._21, m._22, m._23 };
        case 2: return (Vector3) { m._31, m._32, m._33 };
        default:
            LOG_ERROR("Invalid row index %d for Matrix3", i);
            return zeros3();
    }
}


Vector3 matrix3_column(Matrix3 m, int i) {
    switch (i) {
        case 0: return (Vector3) { m._11, m._21, m._31 };
        case 1: return (Vector3) { m._12, m._22, m._32 };
        case 2: return (Vector3) { m._13, m._23, m._33 };
        default:
            LOG_ERROR("Invalid column index %d for Matrix3", i);
            return zeros3();
    }
}


float mat3_get(Matrix3 m, int i, int j) {
    if (i == 0) {
        if (j == 0) return m._11;
        if (j == 1) return m._12;
        if (j == 2) return m._13;
    } else if (i == 1) {
        if (j == 0) return m._21;
        if (j == 1) return m._22;
        if (j == 2) return m._23;
    } else if (i == 2) {
        if (j == 0) return m._31;
        if (j == 1) return m._32;
        if (j == 2) return m._33;
    }
    LOG_ERROR("Invalid indices (%d, %d) for Matrix3", i, j);
    return 0.0f;
}
