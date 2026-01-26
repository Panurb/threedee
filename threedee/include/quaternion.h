#pragma once

#include <stdbool.h>


typedef struct Quaternion {
    float x;
    float y;
    float z;
    float w;
} Quaternion;

typedef struct {
    float yaw;
    float pitch;
    float roll;
} EulerAngles;


Quaternion quaternion_id();

Quaternion quaternion_normalize(Quaternion q);

Quaternion quaternion_conjugate(Quaternion q);

bool quaternion_equals(Quaternion a, Quaternion b);

bool quaternion_non_zero(Quaternion q);

float quaternion_angle(Quaternion q, Quaternion p);

Quaternion quaternion_mult(Quaternion a, Quaternion b);

EulerAngles quaternion_to_euler(Quaternion q);

Quaternion euler_to_quaternion(EulerAngles euler);

Quaternion slerp(Quaternion a, Quaternion b, float t);

Quaternion random_y_rotation();

Quaternion random_z_rotation();
