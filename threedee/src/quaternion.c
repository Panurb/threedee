#define _USE_MATH_DEFINES

#include "quaternion.h"

#include <math.h>
#include <util.h>


Quaternion quaternion_id() {
    return (Quaternion) { 0.0f, 0.0f, 0.0f, 1.0f };
}


Quaternion quaternion_normalize(Quaternion q) {
    float norm = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (norm < 1e-6f) {
        return quaternion_id();
    }
    return (Quaternion) { q.x / norm, q.y / norm, q.z / norm, q.w / norm };
}


Quaternion quaternion_conjugate(Quaternion q) {
    return (Quaternion) { -q.x, -q.y, -q.z, q.w };
}


bool quaternion_equals(Quaternion a, Quaternion b) {
    float epsilon = 1e-3f;
    bool equal = false;

    equal = fabsf(a.x - b.x) < epsilon &&
            fabsf(a.y - b.y) < epsilon &&
            fabsf(a.z - b.z) < epsilon &&
            fabsf(a.w - b.w) < epsilon;

    if (!equal) {
        equal = fabsf(a.x + b.x) < epsilon &&
                fabsf(a.y + b.y) < epsilon &&
                fabsf(a.z + b.z) < epsilon &&
                fabsf(a.w + b.w) < epsilon;
    }

    return equal;
}


bool quaternion_non_zero(Quaternion q) {
    return (q.x != 0.0f || q.y != 0.0f || q.z != 0.0f || q.w != 0.0f);
}


float quaternion_angle(Quaternion q, Quaternion p) {
    // Calculate the angle between two quaternions
    float dot = q.x * p.x + q.y * p.y + q.z * p.z + q.w * p.w;
    if (dot < 0.0f) {
        dot = -dot; // Ensure dot product is non-negative
    }
    return 2.0f * acosf(clamp(dot, -1.0f, 1.0f));
}


Quaternion quaternion_mult(Quaternion a, Quaternion b) {
    Quaternion q;
    q.x = a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x;
    q.y = -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y;
    q.z = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
    q.w = -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w;
    return q;
}


EulerAngles quaternion_to_euler(Quaternion q) {
    // Extrinsic yaw-pitch-roll (XYZ) convention
    EulerAngles euler;

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    euler.roll = atan2f(sinr_cosp, cosr_cosp);

    float sinp = sqrtf(1.0f + 2.0f * (q.w * q.y + q.x * q.z));
    float cosp = sqrtf(1.0f - 2.0f * (q.w * q.y + q.x * q.z));
    euler.pitch = 2.0f * atan2f(sinp, cosp) - M_PI_2;

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    euler.yaw = atan2f(siny_cosp, cosy_cosp);

    euler.roll = to_degrees(euler.roll);
    euler.pitch = to_degrees(euler.pitch);
    euler.yaw = to_degrees(euler.yaw);
    return euler;
}


Quaternion euler_to_quaternion(EulerAngles euler) {
    float roll = to_radians(euler.roll);
    float pitch = to_radians(euler.pitch);
    float yaw = to_radians(euler.yaw);

    // Extrinsic yaw-pitch-roll (XYZ) convention
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);

    Quaternion q;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    q.w = cr * cp * cy + sr * sp * sy;
    return q;
}


Quaternion slerp(Quaternion a, Quaternion b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    // If dot is negative, slerp the opposite quaternion for shortest path
    if (dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }

    // If the quaternions are very close, use linear interpolation
    if (dot > 0.9995f) {
        Quaternion result;
        result.x = a.x + t * (b.x - a.x);
        result.y = a.y + t * (b.y - a.y);
        result.z = a.z + t * (b.z - a.z);
        result.w = a.w + t * (b.w - a.w);
        return quaternion_normalize(result);
    }

    // Proper slerp
    float theta_0 = acosf(dot);
    float sin_theta_0 = sinf(theta_0);

    float theta = theta_0 * t;
    float sin_theta = sinf(theta);

    float s0 = sinf(theta_0 - theta) / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    Quaternion result;
    result.x = s0 * a.x + s1 * b.x;
    result.y = s0 * a.y + s1 * b.y;
    result.z = s0 * a.z + s1 * b.z;
    result.w = s0 * a.w + s1 * b.w;

    return quaternion_normalize(result);
}
