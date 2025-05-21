#include <math.h>
#include <string.h>

#include "component.h"
#include "animation.h"
#include "image.h"
#include "game.h"


int animation_frames(Filename image) {
    if (strcmp(image, "arms_axe") == 0) {
        return 6;
    }  else if (strcmp(image, "arms_sword") == 0) {
        return 8;
    } else if (strcmp(image, "big_boy") == 0) {
        return 2;
    } else if (strcmp(image, "boss_body") == 0) {
        return 4;
    }

    return 1;
}


void animate(float time_step) {
    for (int i = 0; i < game_data->components->entities; i++) {
        ImageComponent* image = ImageComponent_get(i);
        if (!image) continue;

        image->stretch_speed -= 5.0f * image->stretch + 0.1f * image->stretch_speed;
        image->stretch += image->stretch_speed * time_step;

        AnimationComponent* animation = AnimationComponent_get(i);
        if (!animation) continue;

        if (animation->wind_factor > 0.0f) {
            image->offset = sum(image->offset, mult(-animation->wind_factor * time_step, game_data->wind));
            image->offset.x = mod(image->offset.x, resources.texture_sizes[image->texture_index].w);
            image->offset.y = mod(image->offset.y, resources.texture_sizes[image->texture_index].h);
            continue;
        }

        PhysicsComponent* physics = PhysicsComponent_get(i);
        if (physics) {
            animation->framerate = physics->speed;
        }

        JointComponent* joint = JointComponent_get(i);
        if (joint) {
            animation->framerate = AnimationComponent_get(joint->parent)->framerate;
        }

        if (animation->framerate == 0.0f) continue;

        animation->timer += time_step;
        float frame_time = 1.0f / animation->framerate;
        if (animation->timer > frame_time) {
            animation->current_frame++;
            animation->timer -= frame_time;
        }
        if (animation->current_frame >= animation->frames) {
            animation->current_frame = 0;
            if (animation->play_once) {
                animation->framerate = 0.0f;
            }
        }
    }
}


void stop_animation(int entity) {
    AnimationComponent* animation = AnimationComponent_get(entity);
    animation->current_frame = 0;
    animation->framerate = 0.0f;
    animation->timer = 0.0f;
}
