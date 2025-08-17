#pragma once

#include "util.h"


typedef struct {
    Color fog_color;
    float fog_start;
    float fog_end;
    float ambient_light;
} WeatherParameters;


typedef struct {
    Color fog_color;
    float fog_start;
    float fog_end;
    float ambient_light;
} WeatherComponent;


WeatherComponent* WeatherComponent_add(int entity, WeatherParameters parameters);

void WeatherComponent_remove(int entity);
