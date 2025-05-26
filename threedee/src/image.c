#include <stdio.h>
#include <string.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include "app.h"
#include "image.h"
#include "camera.h"
#include "util.h"
#include "component.h"
#include "perlin.h"
#include "particle.h"
#include "animation.h"
#include "game.h"
#include "list.h"


float image_width(int entity) {
    Vector2 scale = get_scale(entity);
    ImageComponent* image = ImageComponent_get(entity);
    return scale.x * image->width;
}


float image_height(int entity) {
    Vector2 scale = get_scale(entity);
    ImageComponent* image = ImageComponent_get(entity);
    return scale.y * image->height;
}


Resolution get_texture_size(Filename filename) {
    if (filename[0] == '\0') {
        return (Resolution) { 0, 0 };
    }
    LOG_INFO("Loading texture from %s", filename);
    Resolution resolution;
    int index = get_texture_index(filename);
    LOG_INFO("Texture index is %d", index);
    resolution.w = resources.textures[index]->w;
    resolution.h = resources.textures[index]->h;
    LOG_INFO("Texture size: %dx%d", resolution.w, resolution.h);
    return resolution;
}


void set_pixel(SDL_Surface *surface, int x, int y, Color color) {

    const Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), NULL, color.r, color.g, color.b, color.a);
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                                + y * surface->pitch
                                                + x * SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel);
    *target_pixel = pixel;
}


Color get_pixel(SDL_Surface *surface, int x, int y) {
    if (x < 0 || y < 0 || x >= surface->w || y >= surface->h) {
        return (Color) { 0, 0, 0, 0 };
    }

    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                                + y * surface->pitch
                                                + x * SDL_GetPixelFormatDetails(surface->format)->bytes_per_pixel);
    Uint32 pixel = *target_pixel;
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(surface->format), NULL, &r, &g, &b, &a);
    return (Color) { r, g, b, a };
}


int create_decal(Vector2 pos, Filename filename, float lifetime) {
    int i = create_entity();
    CoordinateComponent_add(i, pos, rand_angle())->lifetime = lifetime;
    ImageComponent_add(i, filename, 0.0f, 0.0f, LAYER_DECALS);

    return i;
}


void load_textures() {
    LOG_INFO("Loading textures");

    resources.textures_size = list_files_alphabetically("data/images/*.png", resources.texture_names);
    LOG_INFO("Found %d textures", resources.textures_size);

    for (int i = 0; i < resources.textures_size; i++) {
        String path;
        snprintf(path, sizeof(path), "%s%s%s", "data/images/", resources.texture_names[i], ".png");

        SDL_Texture* texture = IMG_LoadTexture(app.renderer, path);

        resources.textures[i] = texture;
    }

    for (int i = 0; i < resources.textures_size; i++) {
        Resolution texture_size = get_texture_size(resources.texture_names[i]);
        texture_size.w /= PIXELS_PER_UNIT;
        texture_size.h /= PIXELS_PER_UNIT;
        resources.texture_sizes[i] = texture_size;
    }

    LOG_INFO("Loaded textures");
}


int get_texture_index(Filename filename) {
    return binary_search_filename(filename, resources.texture_names, resources.textures_size);
}


void set_texture(ImageComponent* image) {
    image->texture_index = get_texture_index(image->filename);
}


void draw_image(int entity, int camera) {
    ImageComponent* image = ImageComponent_get(entity);
    if (image->alpha == 0.0f) {
        return;
    }

    Vector2 scale = get_scale_interpolated(entity, app.delta);
    float stretch_extrapolated = image->stretch + app.delta * app.time_step * image->stretch_speed;

    scale.x *= 1.0f - stretch_extrapolated;
    scale.y *= 1.0f + stretch_extrapolated;

    Vector2 pos = get_position_interpolated(entity, app.delta);
    float w = scale.x * image->width;
    float h = scale.y * image->height;
    float r = sqrtf(w * w + h * h);
    if (!on_screen(camera, pos, r, r)) {
        return;
    }
    

    AnimationComponent* animation = AnimationComponent_get(entity);

    float angle = get_angle_interpolated(entity, app.delta);
    if (image->tile) {
        draw_tiles(camera, image->texture_index, w, h, image->offset, pos, angle, ones2(), image->alpha);
        return;
    }

    int offset = 0;
    if (animation) {
        offset = animation->current_frame;
    }
    draw_sprite(camera, image->texture_index, image->width, image->height, offset, pos, angle, scale, image->alpha);
}


void draw_images(int camera) {
    ListNode* node;
    FOREACH(node, game_data->components->image.order) {
        int i = node->value;

        ImageComponent* image = ImageComponent_get(i);

        if (image->layer <= LAYER_DECALS) continue;
        if (image->layer >= LAYER_ROOFS) break;

        draw_image(i, camera);
    }
}


void change_texture(int entity, Filename filename, float width, float height) {
    ImageComponent* image = ImageComponent_get(entity);
    strcpy(image->filename, filename);
    image->width = width;
    image->height = height;

    if (width == 0.0f || height == 0.0f) {
        Resolution resolution = get_texture_size(filename);
        image->width = resolution.w / (float) PIXELS_PER_UNIT;
        image->height = resolution.h / (float) PIXELS_PER_UNIT;
    }
    
    if (filename[0] == '\0') {
        image->alpha = 0.0f;
    } else {
        image->alpha = 1.0f;
    }
    image->texture_index = get_texture_index(filename);

    if (strstr(filename, "tile") != NULL) {
        image->tile = true;
    } else {
        image->tile = false;
    }

    AnimationComponent* animation = AnimationComponent_get(entity);
    if (animation) {
        animation->frames = animation_frames(filename);
    }
}


void change_layer(int entity, Layer layer) {
    List* image_order = game_data->components->image.order;
    List_remove(image_order, entity);

    if (image_order->size == 0 || ImageComponent_get(image_order->head->value)->layer > layer) {
        List_add(image_order, entity);
    } else {
        ListNode* node;
        FOREACH(node, image_order) {
            if (!node->next || ImageComponent_get(node->next->value)->layer > layer) {
                List_insert(image_order, node, entity);
                break;
            }
        }
    }

    ImageComponent_get(entity)->layer = layer;
}


// void color_pixel(sfUint8* pixels, int width, int x, int y, Color color, float alpha) {
//     int k = (x + y * width) * 4;

//     pixels[k] = color.r;
//     pixels[k + 1] = color.g;
//     pixels[k + 2] = color.b;
//     pixels[k + 3] = color.a * alpha;
// }


// void create_noise(sfUint8* pixels, int width, int height, Vector2f origin, Color color, float sharpness, Permutation p) {
//     for (int i = 0; i < width; i++) {
//         for (int j = 0; j < height; j++) {
//             float x = origin.x + 4.0 * i / (float) PIXELS_PER_UNIT;
//             float y = origin.y + 4.0 * (height - j) / (float) PIXELS_PER_UNIT;
//             float a = octave_perlin(x, y, 0.0, p, 8, 4, 0.5);

//             if (sharpness != 0.0f) {
//                 a = smoothstep(a, 0.5f, 100.0f * sharpness);
//             }

//             color_pixel(pixels, width, i, j, color, a);
//         }
//     }
// }


bool point_inside_image(int entity, Vector2 point) {
    Vector2 position = get_xy(entity);
    float angle = get_angle(entity);
    ImageComponent* image = ImageComponent_get(entity);
    return point_inside_rectangle(position, angle, image_width(entity), image_height(entity), point);
}
