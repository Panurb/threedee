#include "systems/particle.h"

#include "render.h"
#include "util.h"
#include "scene.h"



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


void add_particles(Entity entity, int count) {
    ParticleComponent* particle = get_component(entity, COMPONENT_PARTICLE);
    if (!particle) return;

    Vector3 position = get_position(entity);

    for (int i = 0; i < count; i++) {
        if (particle->num_particles >= MAX_PARTICLES) {
            break;
        }
        particle->position[particle->num_particles] = add3(
            position,
            vec3(
                randf(-0.5f, 0.5f) * particle->position_variance.x,
                randf(-0.5f, 0.5f) * particle->position_variance.y,
                randf(-0.5f, 0.5f) * particle->position_variance.z
            )
        );
        particle->velocity[particle->num_particles] = add3(
            particle->spawn_velocity,
            vec3(
                randf(-0.5f, 0.5f) * particle->velocity_variance.x,
                randf(-0.5f, 0.5f) * particle->velocity_variance.y,
                randf(-0.5f, 0.5f) * particle->velocity_variance.z
            )
        );
        particle->time[particle->num_particles] = 0.0f;
        particle->num_particles++;
    }
}


void update_particles(float time_step) {
    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        ParticleComponent* particle = get_component(entity, COMPONENT_PARTICLE);
        if (!particle) continue;

        Vector3 position = get_position(entity);

        for (int i = 0; i < particle->num_particles; i++) {
            particle->time[i] += time_step;
            if (particle->time[i] >= particle->lifetime) {
                // Remove particle by swapping with the last one
                particle->position[i] = particle->position[particle->num_particles - 1];
                particle->velocity[i] = particle->velocity[particle->num_particles - 1];
                particle->time[i] = particle->time[particle->num_particles - 1];
                particle->num_particles--;
                i--; // Check the swapped particle
                continue;
            }

            particle->position[i] = add3(
                particle->position[i],
                mul3(time_step, particle->velocity[i])
            );

            particle->velocity[i] = add3(
                particle->velocity[i],
                mul3(time_step * particle->gravity_scale, scene->gravity)
            );
        }

        if (particle->spawn_rate <= 0.0f) {
            continue;
        }

        particle->spawn_accumulator += particle->spawn_rate * time_step;

        if (particle->spawn_accumulator > 1.0f) {
            int n = (int)particle->spawn_accumulator;
            add_particles(entity, n);
            particle->spawn_accumulator -= n;
        }
    }
}


void draw_particles() {
    for (Entity entity = 0; entity < scene->components->entities; entity++) {
        ParticleComponent* particle = scene->components->particle[entity];
        if (!particle) continue;

        for (int i = 0; i < particle->num_particles; i++) {
            float normalized_time = particle->time[i] / particle->lifetime;

            // Default to zero size fading at ends
            ParticlePhase previous_phase = {
                .color = particle->phases[0].color,
                .normalized_time = 0.0f,
                .size = 0.0f
            };
            ParticlePhase next_phase = particle->phases[0];

            for (int j = 0; j < particle->num_phases; j++) {
                if (normalized_time > particle->phases[j].normalized_time) {
                    previous_phase = particle->phases[j];
                    if (j < particle->num_phases - 1) {
                        next_phase = particle->phases[j + 1];
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

            float speed = norm3(particle->velocity[i]);
            Vector3 direction = div3(speed, particle->velocity[i]);
            Vector3 camera_up = get_axes(scene->camera).up;
            float angle = atan2f(
                dot3(camera_up, cross(camera_up, direction)),
                dot3(camera_up, direction)
            );

            draw_particle(
                particle->position[i],
                blended_phase.size,
                blended_phase.size * (1.0f + speed * particle->stretch),
                angle,
                particle->texture_index,
                blended_phase.color,
                particle->emissive
            );
        }
    }
}
