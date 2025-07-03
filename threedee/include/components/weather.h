#pragma once

#include "util.h"


typedef struct {
    Color fog_color;
    float fog_start;
    float fog_end;
} WeatherParameters;


typedef struct {
    Color fog_color;
    float fog_start;
    float fog_end;
} WeatherComponent;


WeatherComponent* WeatherComponent_add(int entity, WeatherParameters parameters);

void WeatherComponent_remove(int entity);
