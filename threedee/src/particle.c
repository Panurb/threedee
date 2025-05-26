#define _USE_MATH_DEFINES
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "app.h"
#include "particle.h"
#include "component.h"
#include "util.h"
#include "camera.h"
#include "game.h"
#include "settings.h"


Color color_lerp(Color s, Color e, float t) {
    Color color;
    color.r = lerp(s.r, e.r, t);
    color.g = lerp(s.g, e.g, t);
    color.b = lerp(s.b, e.b, t);
    color.a = lerp(s.a, e.a, t);
    return color;
}


void ParticleComponent_add_bullet(int entity, float size) {
    ParticleComponent_add(entity, 0.0f, 0.0f, size, size, 200.0f, 1, get_color(1.0f, 1.0f, 0.5f, 0.5f), COLOR_WHITE)->speed_spread = 0.0f;
}


void ParticleComponent_add_blood(int entity) {
    Color color = get_color(0.78, 0.0, 0.0, 1.0);
    Color inner_color = color_lerp(color, COLOR_WHITE, 0.25f);
    ParticleComponent_add(entity, 0.0, 2 * M_PI, 0.3f, 0.0f, 3.0f, 10.0f, color, inner_color);
}


void ParticleComponent_add_sparks(int entity) {
    ParticleComponent_add(entity, 0.0, 2 * M_PI, 0.15, 0.0, 5.0, 5.0, get_color(1.0f, 1.0f, 0.5f, 0.5f), COLOR_WHITE);
}


void ParticleComponent_add_dirt(int entity) {
    Color color = get_color(0.4, 0.25, 0.13, 0.5f);
    Color inner_color = color_lerp(color, COLOR_WHITE, 0.5f);
    ParticleComponent_add(entity, 0.0, 2 * M_PI, 0.25, 0.0, 2.5, 10.0, color, inner_color);
}


void ParticleComponent_add_rock(int entity) {
    Color color = get_color(0.4f, 0.4f, 0.4f, 1.0f);
    Color inner_color = color_lerp(color, COLOR_WHITE, 0.5f);
    ParticleComponent_add(entity, 0.0f, 2.0f * M_PI, 0.3f, 0.0f, 1.5f, 5.0f, color, inner_color);
}


void ParticleComponent_add_splinter(int entity) {
    Color color = get_color(0.5f, 0.4f, 0.3f, 1.0f);
    Color inner_color = color_lerp(color, COLOR_WHITE, 0.5f);
    ParticleComponent_add(entity, 0.0f, 2.0f * M_PI, 0.15f, 0.0f, 5.0f, 10.0f, color, inner_color);
}


void ParticleComponent_add_fire(int entity, float size) {
    float angle = 0.5f * M_PI - get_angle(entity);
    ParticleComponent* particle = ParticleComponent_add(entity, angle, 1.0, size, 0.25f * size, 1.0, 10.0, COLOR_ORANGE, COLOR_YELLOW);
    particle->loop = true;
    particle->enabled = true;
}


void ParticleComponent_add_energy(int entity) {
    Color green = get_color(0.5f, 1.0f, 0.0f, 1.0f);
    ParticleComponent_add(entity, 0.0, 2.0 * M_PI, 0.1, 0.05, 2.0, 5.0, green, green);
}


void ParticleComponent_add_glass(int entity) {
    Color color = get_color(0.8f, 0.8f, 0.8f, 0.5f);
    Color inner_color = color_lerp(color, COLOR_WHITE, 0.5f);
    ParticleComponent_add(entity, 0.0, 2.0 * M_PI, 0.15, 0.0, 5.0, 10.0, color, inner_color);
}


void ParticleComponent_add_snow(int entity) {
    ParticleComponent* part = ParticleComponent_add(entity, 0.0, 2 * M_PI, 0.1, 0.0, 0.5, 10.0, COLOR_WHITE, COLOR_NONE);
    part->enabled = true;
    part->loop = true;
    part->max_time = 20.0f;
}


void ParticleComponent_add_rain(int entity) {
    Color color = get_color(0.8f, 0.8f, 1.0f, 0.5f);
    ParticleComponent* part = ParticleComponent_add(entity, -2.0f, 0.1, 0.05, 0.0, 1.0, 50.0, color, COLOR_NONE);
    part->enabled = true;
    part->loop = true;
    part->max_time = 0.5f;
    part->stretch = 10.0f;
    part->speed_spread = 0.0f;
}


void ParticleComponent_add_steam(int entity, float size) {
    Color color = get_color(1.0f, 1.0f, 1.0f, 0.5f);
    ParticleComponent* part = ParticleComponent_add(entity, 0.0, 0.5 * M_PI, size, 0.0, 1.0, 4.0, color, COLOR_WHITE);
    part->enabled = true;
    part->loop = true;
    part->max_time = 2.0f;
}


void ParticleComponent_add_water(int entity, float size) {
    Color color = get_color(0.5f, 0.5f, 1.0f, 0.5f);
    Color inner_color = get_color(0.5f, 0.5f, 1.0f, 1.0f);
    ParticleComponent* particle = ParticleComponent_add(entity, 0.0f, 1.0, size, 0.25f * size, 1.0, 10.0, color, inner_color);
    particle->loop = true;
    particle->enabled = true;
}


void ParticleCompoenent_add_radiation(int entity) {
    ParticleComponent* part = ParticleComponent_add(entity, 0.0, 2.0f * M_PI, 0.1, 0.05, 2.0, 5.0, COLOR_ENERGY, COLOR_NONE);
    part->enabled = true;
    part->loop = true;
    part->wind_factor = 0.0f;
}


void ParticleCompoenent_add_push(int entity) {
    ParticleComponent* part = ParticleComponent_add(entity, M_PI_2, 0.0f, 0.2, 0.05, 2.0f, 20.0f, COLOR_ENERGY, COLOR_NONE);
    part->enabled = true;
    part->loop = true;
    part->wind_factor = 0.0f;
}


ParticleComponent* ParticleComponent_add_type(int entity, ParticleType type, float size) {
    switch (type) {
        case PARTICLE_NONE:
            return NULL;
        case PARTICLE_BULLET:
            ParticleComponent_add_bullet(entity, size);
            break;
        case PARTICLE_BLOOD:
            ParticleComponent_add_blood(entity);
            break;
        case PARTICLE_SPARKS:
            ParticleComponent_add_sparks(entity);
            break;
        case PARTICLE_DIRT:
            ParticleComponent_add_dirt(entity);
            break;
        case PARTICLE_ROCK:
            ParticleComponent_add_rock(entity);
            break;
        case PARTICLE_SPLINTER:
            ParticleComponent_add_splinter(entity);
            break;
        case PARTICLE_FIRE:
            ParticleComponent_add_fire(entity, size);
            break;
        case PARTICLE_ENERGY:
            ParticleComponent_add_energy(entity);
            break;
        case PARTICLE_GLASS:
            ParticleComponent_add_glass(entity);
            break;
        case PARTICLE_SNOW:
            ParticleComponent_add_snow(entity);
            break;
        case PARTICLE_RAIN:
            ParticleComponent_add_rain(entity);
            break;
        case PARTICLE_STEAM:
            ParticleComponent_add_steam(entity, size);
            break;
        case PARTICLE_WATER:
            ParticleComponent_add_water(entity, size);
            break;
        case PARTICLE_RADIATION:
            ParticleCompoenent_add_radiation(entity);
            break;
        case PARTICLE_PUSH:
            ParticleCompoenent_add_push(entity);
            break;
    }
    ParticleComponent* particle = ParticleComponent_get(entity);
    particle->type = type;
    return particle;
}


void add_particles(int entity, int n) {
    Vector2 scale = get_scale(entity);
    ParticleComponent* part = ParticleComponent_get(entity);

    for (int i = 0; i < n; i++) {
        int next = (part->first + part->particles) % part->max_particles;

        if (part->width > 0.0f && part->height > 0.0f) {
            float dx = 0.5f * part->width * scale.x;
            float dy = 0.5f * part->height * scale.y;
            part->origin = (Vector2) {
                randf(-dx, dx), 
                randf(-dy, dy)
            };
        }
        part->position[next] = sum(get_xy(entity), rotate(part->origin, get_angle(entity)));
        float r = part->speed * randf(1.0 - part->speed_spread, 1.0 + part->speed_spread);
        float angle = randf(part->angle - 0.5 * part->spread, part->angle + 0.5 * part->spread);
        part->velocity[next] = polar_to_cartesian(r, get_angle(entity) + angle);
        part->time[next] = part->max_time;
        
        if (part->particles < part->max_particles) {
            part->particles++;
        } else {
            part->first = (part->first + 1) % part->max_particles;
        }
    }
}


void update_particles(int camera, float time_step) {
    for (int i = 0; i < game_data->components->entities; i++) {
        ParticleComponent* part = ParticleComponent_get(i);
        if (!part) continue;

        float w = 3.0f * part->max_time * part->speed;
        bool visible = on_screen(camera, get_xy(i), w, w);
        if (part->loop && !visible) {
            continue;
        }

        if (part->enabled) {
            if (part->loop) {
                while (part->timer <= 0.0) {
                    add_particles(i, 1);
                    part->timer += 1.0 / part->rate;
                }

                part->timer -= time_step;
            } else {
                add_particles(i, part->rate);
                part->enabled = false;
            }
        }

        for (int j = 0; j < part->particles; j++) {
            int p = (part->first + j) % part->max_particles;
            part->position[p] = sum(part->position[p], mult(time_step, part->velocity[p]));
            part->time[p] = fmax(0.0f, part->time[p] - time_step);
        }

        if (part->particles > 0 && part->time[part->first] == 0.0f) {
            part->first = (part->first + 1) % part->max_particles;
            part->particles--;
        }
    }
}


void draw_particles(int camera) {
    for (int entity = 0; entity < game_data->components->entities; entity++) {
        ParticleComponent* part = ParticleComponent_get(entity);
        if (!part) continue;

        for (int i = 0; i < part->particles; i++) {
            int p = (part->first + part->particles - i - 1) % part->max_particles;

            if (part->time[p] == 0.0) continue;

            Color color = part->outer_color;

            // Always extrapolate particles
            float time = part->time[p] + app.delta * app.time_step;
            Vector2 position = sum(part->position[p], mult(app.delta * app.time_step, part->velocity[p]));

            float t = 1.0f - part->time[p] / part->max_time;
            float r = lerp(part->start_size, part->end_size, t);
            float angle = polar_angle(part->velocity[p]);

            draw_ellipse(camera, position, fmaxf(1.0f, 0.6f * part->stretch * norm2(part->velocity[p])) * r, r, angle, color);
        }

        if (part->inner_color.a == 0.0f) continue;

        for (int i = 0; i < part->particles; i++) {
            int p = (part->first + part->particles - i - 1) % part->max_particles;

            if (part->time[p] == 0.0) continue;

            Color color = part->inner_color;

            // Always extrapolate particles
            float time = part->time[p] + app.delta * app.time_step;
            Vector2 position = sum(part->position[p], mult(app.delta * app.time_step, part->velocity[p]));

            float t = 1.0f - part->time[p] / part->max_time;
            float r = 0.5f * lerp(part->start_size, part->end_size, t);
            float angle = polar_angle(part->velocity[p]);

            draw_ellipse(camera, position, fmaxf(1.0f, part->stretch * norm2(part->velocity[p])) * r, r, angle, color);
        }   
    }
}
