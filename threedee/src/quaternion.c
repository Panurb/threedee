#define _USE_MATH_DEFINES

#include "quaternion.h"

#include <math.h>


Quaternion quaternion_id() {
    return (Quaternion) { 0.0f, 0.0f, 0.0f, 1.0f };
}


Quaternion quaternion_normalize(Quaternion q) {
    float norm = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (norm == 0.0f) {
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
    EulerAngles angles;

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    angles.roll = atan2f(sinr_cosp, cosr_cosp);

    float sinp = sqrtf(1.0f + 2.0f * (q.w * q.y + q.x * q.z));
    float cosp = sqrtf(1.0f - 2.0f * (q.w * q.y + q.x * q.z));
    angles.pitch = 2.0f * atan2f(sinp, cosp) - M_PI_2;

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    angles.yaw = atan2f(siny_cosp, cosy_cosp);

    return angles;
}


Quaternion euler_to_quaternion(EulerAngles euler) {
    // Extrinsic yaw-pitch-roll (XYZ) convention
    float cr = cosf(euler.roll * 0.5f);
    float sr = sinf(euler.roll * 0.5f);
    float cp = cosf(euler.pitch * 0.5f);
    float sp = sinf(euler.pitch * 0.5f);
    float cy = cosf(euler.yaw * 0.5f);
    float sy = sinf(euler.yaw * 0.5f);

    Quaternion q;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;
    q.w = cr * cp * cy + sr * sp * sy;
    return q;
}
