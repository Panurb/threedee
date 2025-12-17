#include "systems/particle.h"

#include <assert.h>
#include <stdio.h>

#include "render.h"
#include "util.h"
#include "scene.h"
#include "components/emitter.h"



ParticleData* ParticleData_create() {
    ParticleData* pd = malloc(sizeof(ParticleData));
    pd->size = 0;
    return pd;
}


ParticlePhase lerp_particle_phase(ParticlePhase a, ParticlePhase b, float t) {
    ParticlePhase result;
    result.color.r = lerp(a.color.r, b.color.r, t);
    result.color.g = lerp(a.color.g, b.color.g, t);
    result.color.b = lerp(a.color.b, b.color.b, t);
    result.color.a = lerp(a.color.a, b.color.a, t);
    result.size = lerp(a.size, b.size, t);
    result.normalized_time = lerp(a.normalized_time, b.normalized_time, t);
    return result;
}


void add_particles(int count, Vector3 position, Vector3 velocity, float size, int particle_type) {
    assert(particle_type != -1, "Invalid particle type index");

    ParticleData* particles = scene->particles;

    for (int i = 0; i < count; i++) {
        if (particles->size >= MAX_PARTICLES) {
            break;
        }
        particles->position[particles->size] = position;
        particles->velocity[particles->size] = velocity;
        particles->scale[particles->size] = size;
        particles->time[particles->size] = 0.0f;
        particles->particle_type[particles->size] = particle_type;
        particles->size++;
    }
}


void update_particles(float time_step) {
    ParticleData* particles = scene->particles;

    for (int i = 0; i < particles->size; i++) {
        ParticleType particle_type = resources.particle_types[particles->particle_type[i]];

        particles->time[i] += time_step;
        if (particles->time[i] >= particle_type.lifetime * particles->scale[i]) {

            // Remove particle by swapping with the last one
            particles->position[i] = particles->position[particles->size - 1];
            particles->velocity[i] = particles->velocity[particles->size - 1];
            particles->time[i] = particles->time[particles->size - 1];
            particles->particle_type[i] = particles->particle_type[particles->size - 1];
            particles->scale[i] = particles->scale[particles->size - 1];
            particles->size--;

            i--; // Check the swapped particle
            continue;
        }

        particles->position[i] = add3(
            particles->position[i],
            mul3(time_step, particles->velocity[i])
        );

        particles->velocity[i] = add3(
            particles->velocity[i],
            mul3(time_step * particle_type.gravity_scale, scene->gravity)
        );
    }
}


void update_emitters(float time_step) {
    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        EmitterComponent* emitter = get_component(entity, COMPONENT_EMITTER);
        if (!emitter) continue;

        Vector3 spawn_position = get_position(entity);

        if (emitter->spawn_rate <= 0.0f) {
            continue;
        }

        emitter->spawn_accumulator += emitter->spawn_rate * time_step;

        if (emitter->spawn_accumulator > 1.0f) {
            int n = (int)emitter->spawn_accumulator;

            Vector3 position = add3(
                spawn_position,
                vec3(
                    randf(-0.5f, 0.5f) * emitter->position_variance.x,
                    randf(-0.5f, 0.5f) * emitter->position_variance.y,
                    randf(-0.5f, 0.5f) * emitter->position_variance.z
                )
            );
            Vector3 velocity = add3(
                emitter->spawn_velocity,
                vec3(
                    randf(-0.5f, 0.5f) * emitter->velocity_variance.x,
                    randf(-0.5f, 0.5f) * emitter->velocity_variance.y,
                    randf(-0.5f, 0.5f) * emitter->velocity_variance.z
                )
            );

            add_particles(n, position, velocity, emitter->scale, emitter->particle_type);
            emitter->spawn_accumulator -= n;
        }
    }
}


void draw_particles() {
    ParticleData* particles = scene->particles;

    for (int i = 0; i < particles->size; i++) {
        ParticleType particle_type = resources.particle_types[particles->particle_type[i]];

        float normalized_time = particles->time[i] / (particle_type.lifetime * particles->scale[i]);

        // Default to zero size fading at ends
        ParticlePhase previous_phase = {
            .color = particle_type.phases[0].color,
            .normalized_time = 0.0f,
            .size = 0.0f
        };
        ParticlePhase next_phase = particle_type.phases[0];

        for (int j = 0; j < particle_type.num_phases; j++) {
            if (normalized_time > particle_type.phases[j].normalized_time) {
                previous_phase = particle_type.phases[j];
                if (j < particle_type.num_phases - 1) {
                    next_phase = particle_type.phases[j + 1];
                } else {
                    next_phase.size = 0.0f;
                    next_phase.normalized_time = 1.0f;
                }
            }
        }

        ParticlePhase blended_phase = lerp_particle_phase(
            previous_phase,
            next_phase,
            (normalized_time - previous_phase.normalized_time) /
            (next_phase.normalized_time - previous_phase.normalized_time)
        );

        float speed = norm3(particles->velocity[i]);
        Vector3 direction = div3(speed, particles->velocity[i]);
        Vector3 camera_up = get_axes(scene->camera).up;
        float angle = atan2f(
            dot3(camera_up, cross(camera_up, direction)),
            dot3(camera_up, direction)
        );

        float size = blended_phase.size * particles->scale[i];
        float area = size * size;
        float height = size * (1.0f + speed * particle_type.stretch);

        draw_particle(
            particles->position[i],
            area / height,
            height,
            angle,
            particle_type.texture_index,
            blended_phase.color,
            particle_type.emissive
        );
    }
}
