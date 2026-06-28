#pragma once

#include "util.h"
#include "linalg.h"
#include "components/collider.h"


typedef struct ForceParameters {
    Vector3 direction;
    float magnitude;
    ColliderGroup target_group;
    bool disabled;
} ForceParameters;


typedef struct ForceComponent {
    bool enabled;
    Vector3 direction;
    float magnitude;
    ColliderGroup target_group;
} ForceComponent;


ForceComponent* ForceComponent_add(Entity entity, ForceParameters params);


void ForceComponent_remove(Entity entity);
