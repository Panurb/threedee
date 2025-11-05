#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "components/sprite.h"
#include "scene.h"
#include "resources.h"


SpriteComponent* SpriteComponent_add(Entity entity, SpriteParameters params) {
    SpriteComponent* sprite = malloc(sizeof(SpriteComponent));
    sprite->width = params.width ? params.width : 1.0f;
    sprite->height = params.height ? params.height : 1.0f;
    sprite->uv_top_left = params.uv_top_left;
    sprite->uv_bottom_right = non_zero2(params.uv_bottom_right) ? params.uv_bottom_right : ones2();
    sprite->texture_index = -1;

    if (params.texture_filename[0] != '\0') {
        sprite->texture_index = binary_search_filename(params.texture_filename, resources.texture_names, resources.textures_size);
        if (sprite->texture_index == -1) {
            LOG_ERROR("Texture not found: %s", params.texture_filename);
        }
    }

    scene->components->sprite[entity] = sprite;
    return sprite;
}


void SpriteComponent_remove(Entity entity) {
    SpriteComponent* sprite = get_component(entity, COMPONENT_SPRITE);
    if (sprite) {
        free(sprite);
        scene->components->sprite[entity] = NULL;
    }
}
