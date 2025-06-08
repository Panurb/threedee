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

bool quaternion_equals(Quaternion a, Quaternion b);

Quaternion quaternion_mult(Quaternion a, Quaternion b);

EulerAngles quaternion_to_euler(Quaternion q);

Quaternion euler_to_quaternion(EulerAngles euler);
