#include "components/weather.h"
#include "scene.h"


WeatherComponent* WeatherComponent_add(int entity, WeatherParameters parameters) {
    WeatherComponent* weather = malloc(sizeof(WeatherComponent));
    weather->fog_color = parameters.fog_color;
    weather->fog_start = parameters.fog_start;
    weather->fog_end = parameters.fog_end;

    scene->components->weather[entity] = weather;

    return weather;
}


void WeatherComponent_remove(int entity) {
    WeatherComponent* weather = scene->components->weather[entity];
    if (weather) {
        free(weather);
        scene->components->weather[entity] = NULL;
    }
}
