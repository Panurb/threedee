#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "app.h"
#include "camera.h"
#include "component.h"
#include "util.h"
#include "image.h"
#include "game.h"
#include "settings.h"


SDL_FColor color_to_fcolor(Color color) {
    return (SDL_FColor) { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}


int create_camera() {
    int i = create_entity();
    CoordinateComponent_add(i, zeros(), 0.0);
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, 40.0f);
    return i;
}


int create_menu_camera() {
    int i = create_entity();
    CoordinateComponent_add(i, zeros(), 0.0);
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, 25.0f);
    return i;
}


Vector2f camera_size(int camera) {
    CameraComponent* cam = CameraComponent_get(camera);
    Resolution res = cam->resolution;
    float zoom = cam->zoom;
    return (Vector2f) { res.w / zoom, res.h / zoom };
}


Vector2f world_to_screen(int camera, Vector2f a) {
    CameraComponent* cam = CameraComponent_get(camera);
    Vector2f pos = sum(get_position_interpolated(camera, app.delta), cam->shake.position);
    Vector2f b;
    b.x = (a.x - pos.x) * cam->zoom + 0.5 * cam->resolution.w;
    b.y = (pos.y - a.y) * cam->zoom + 0.5 * cam->resolution.h;
    return b;
}


Vector2f screen_to_world(int camera, Vector2f a) {
    CameraComponent* cam = CameraComponent_get(camera);
    Vector2f pos = sum(get_position_interpolated(camera, app.delta), cam->shake.position);
    Vector2f b;
    b.x = (a.x - 0.5 * cam->resolution.w) / cam->zoom + pos.x;
    b.y = (0.5 * cam->resolution.h - a.y) / cam->zoom + pos.y;
    return b;
}


SDL_FRect world_to_screen_rect(int camera, Vector2f position, float width, float height) {
    Vector2f pos = world_to_screen(camera, position);
    CameraComponent* cam = CameraComponent_get(camera);
    float w = width * cam->zoom;
    float h = height * cam->zoom;
    return (SDL_FRect) { pos.x - 0.5f * w, pos.y - 0.5f * h, w, h };
}


void draw_triangle_fan(int camera, SDL_Vertex* vertices, int verts_size) {
    int* indices = malloc(3 * verts_size * sizeof(int));
    for (int i = 0; i < verts_size; i++) {
        indices[3 * i] = 0;
        indices[3 * i + 1] = i;
        indices[3 * i + 2] = (i + 1) % verts_size;
    }
    SDL_RenderGeometry(app.renderer, NULL, vertices, verts_size, indices, 3 * verts_size);
    free(indices);
}


void draw_triangle_strip(int camera, int texture_index, SDL_Vertex* vertices, int verts_size) {
    int* indices = malloc(3 * (verts_size - 2) * sizeof(int));
    for (int i = 0; i < verts_size - 2; i++) {
        indices[3 * i] = i;
        indices[3 * i + 1] = i + 1;
        indices[3 * i + 2] = i + 2;
    }

    SDL_Texture* texture = NULL;
    if (texture_index != -1) {
        texture = resources.textures[texture_index];
    }

    SDL_RenderGeometry(app.renderer, texture, vertices, verts_size, indices, 3 * (verts_size - 2));
    free(indices);
}


void draw_line(int camera, Vector2f start, Vector2f end, float width, Color color) {
    CameraComponent* cam = CameraComponent_get(camera);

    // Prevent lines from disappearing when zoomed out
    if (width * cam->zoom < 1.0f) {
        width = 1.0f / cam->zoom;
    }
    Vector2f r = diff(end, start);

    Vector2f corners[4];
    get_rect_corners(sum(start, mult(0.5f, r)), atan2(r.y, r.x), dist(start, end), width, corners);

    SDL_Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        Vector2f v = world_to_screen(camera, corners[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor(color);
    }

    int indices[6] = { 0, 1, 2, 2, 3, 0 };
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(app.renderer, NULL, vertices, 4, indices, 6);
}


void draw_circle(int camera, Vector2f position, float radius, Color color) {
    int points_size = clamp(20 * radius, 20, 100);
    Vector2f points[100];
    get_circle_points(position, radius, points_size, points);

    SDL_Vertex vertices[101];
    for (int i = 0; i < points_size; i++) {
        Vector2f v = world_to_screen(camera, points[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor(color);
    }
    vertices[points_size] = vertices[1];

    draw_triangle_fan(camera, vertices, points_size + 1);
}


void draw_ellipse_two_color(int camera, Vector2f position, float major, float minor, float angle, 
        Color color, Color color_center) {
    int points_size = clamp(20 * major, 20, 100);
    Vector2f points[100];
    get_ellipse_points(position, major, minor, angle, 20, points);

    SDL_Vertex vertices[100];
    for (int i = 0; i < points_size; i++) {
        Vector2f v = world_to_screen(camera, points[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor(color);
    }
    vertices[0].color = color_to_fcolor(color_center);

    draw_triangle_fan(camera, vertices, points_size);
}


void draw_circle_outline(int camera, Vector2f position, float radius, float line_width, Color color) {
    int points_size = clamp(20 * radius, 20, 100);
    Vector2f points[100];
    get_circle_points(position, radius, points_size, points);
    points[0] = points[points_size - 1];

    for (int i = 0; i < points_size; i++) {
        draw_line(camera, points[i], points[(i + 1) % points_size], line_width, color);
    }
}


void draw_ellipse(int camera, Vector2f position, float major, float minor, float angle, Color color) {
    Vector2f points[20];
    get_ellipse_points(position, major, minor, angle, 20, points);

    SDL_Vertex vertices[21];
    for (int i = 0; i < 20; i++) {
        Vector2f v = world_to_screen(camera, points[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor(color);
    }
    vertices[20] = vertices[1];

    draw_triangle_fan(camera, vertices, 21);
}


void draw_rectangle(int camera, Vector2f position, float width, float height, float angle, Color color) {
    Vector2f corners[4];
    get_rect_corners(position, angle, width, height, corners);

    SDL_Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        Vector2f v = world_to_screen(camera, corners[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor(color);
    }

    int indices[6] = { 0, 1, 2, 2, 3, 0 };
    SDL_RenderGeometry(app.renderer, NULL, vertices, 4, indices, 6);
}



void draw_rectangle_outline(int camera, Vector2f position, float width, float height, 
        float angle, float line_width, Color color) {
    Vector2f corners[4];
    get_rect_corners(position, angle, width, height, corners);

    for (int i = 0; i < 4; i++) {
        draw_line(camera, corners[i], corners[(i + 1) % 4], line_width, color);
    }
}


void draw_slice(int camera, Vector2f position, float min_range, float max_range, 
        float angle, float spread, Color color) {
    int points = 10 * ceil(max_range * spread);
    SDL_Vertex* vertices = malloc(points * sizeof(SDL_Vertex));

    Vector2f start = polar_to_cartesian(min_range, angle - 0.5 * spread);
    Vector2f end = polar_to_cartesian(max_range, angle - 0.5 * spread);

    Matrix2f rot = rotation_matrix(spread / (points / 2.0f - 1));

    for (int k = 0; k < points / 2; k += 1) {
        Vector2f pos = world_to_screen(camera, sum(position, start));
        vertices[2 * k].position = (SDL_FPoint) { pos.x, pos.y };
        vertices[2 * k].color = color_to_fcolor(color);
        
        pos = world_to_screen(camera, sum(position, end));
        vertices[2 * k + 1].position = (SDL_FPoint) { pos.x, pos.y };
        vertices[2 * k + 1].color = color_to_fcolor(color);

        start = matrix_mult(rot, start);
        end = matrix_mult(rot, end);
    }
    draw_triangle_strip(camera, -1, vertices, points);
    free(vertices);
}


void draw_arc(int camera, Vector2f position, float range, float angle, float spread) {
    int n = 5 * ceil(range * spread);

    Matrix2f rot = rotation_matrix(spread / n);

    Vector2f start = polar_to_cartesian(range, angle - 0.5 * spread);
    Vector2f end = start;

    for (int k = 0; k < n; k++) {
        end = matrix_mult(rot, end);
        draw_line(camera, sum(position, start), sum(position, end), 0.05, COLOR_WHITE);
        start = end;
    }
}


void draw_slice_outline(int camera, Vector2f position, float min_range, float max_range, float angle, float spread) {
    Vector2f start = polar_to_cartesian(max_range, angle - 0.5 * spread);
    Vector2f end = mult(min_range / max_range, start);

    draw_line(camera, sum(position, start), sum(position, end), 0.05, COLOR_WHITE);

    draw_arc(camera, position, min_range, angle, spread);

    draw_arc(camera, position, max_range, angle, spread);

    start = polar_to_cartesian(min_range, angle + 0.5 * spread);
    end = mult(max_range / min_range, start);    
    
    draw_line(camera, sum(position, start), sum(position, end), 0.05, COLOR_WHITE);
}


void draw_sprite(int camera, int texture_index, float width, float height, int offset, Vector2f position, float angle,
        Vector2f scale, float alpha) {
    if (texture_index == -1) {
        return;
    }

    // CameraComponent* cam = CameraComponent_get(camera);
    //
    // Vector2f scaling = mult(cam->zoom / PIXELS_PER_UNIT, scale);
    //
    // SDL_Texture* texture = resources.textures[texture_index];
    // SDL_SetTextureAlphaMod(texture, 255 * alpha);
    //
    // SDL_Rect src = { 0, 0, width * PIXELS_PER_UNIT, height * PIXELS_PER_UNIT };
    // if (offset != 0) {
    //     src.x = offset * width * PIXELS_PER_UNIT;
    // }
    //
    // bool tile = false;
    // int texture_width;
    // int texture_height;
    // SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);
    // if (texture_width < width * PIXELS_PER_UNIT || texture_height < height * PIXELS_PER_UNIT) {
    //     tile = true;
    // }
    //
    // if (width == 0.0f || height == 0.0f) {
    //     SDL_QueryTexture(texture, NULL, NULL, &src.w, &src.h);
    //     width = (float)src.w / PIXELS_PER_UNIT;
    //     height = (float)src.h / PIXELS_PER_UNIT;
    // }
    //
    // Vector2f pos = world_to_screen(camera, position);
    //
    // SDL_FRect dest;
    // dest.w = width * scaling.x * PIXELS_PER_UNIT;
    // dest.h = height * scaling.y * PIXELS_PER_UNIT;
    // dest.x = pos.x - 0.5f * dest.w;
    // dest.y = pos.y - 0.5f * dest.h;
    //
    // SDL_Rect* psrc = &src;
    // if (width == 0.0f || height == 0.0f) {
    //     SDL_Rect* psrc = NULL;
    // }
    // SDL_RenderCopyExF(app.renderer, texture, psrc, &dest, -to_degrees(angle), NULL, SDL_FLIP_NONE);
}


void draw_tiles(int camera, int texture_index, float width, float height, Vector2f offset, Vector2f position,
        float angle, Vector2f scale, float alpha) {
    if (texture_index == -1) {
        return;
    }

    // CameraComponent* cam = CameraComponent_get(camera);
    //
    // SDL_Texture* texture = resources.textures[texture_index];
    // SDL_Rect src = { 0, 0, 0, 0 };
    //
    // Resolution texture_size = resources.texture_sizes[texture_index];
    // src.w = texture_size.w * PIXELS_PER_UNIT;
    // src.h = texture_size.h * PIXELS_PER_UNIT;
    // float tile_width = texture_size.w;
    // float tile_height = texture_size.h;
    //
    // if (offset.x > tile_width || offset.y > tile_height || offset.x < 0.0f || offset.y < 0.0f) {
    //     LOG_ERROR("Offset: %f, %f out of bounds: %f, %f", offset.x, offset.y, tile_width, tile_height);
    // }
    //
    // SDL_FRect dest = { 0, 0, 0, 0 };
    //
    // Vector2f x = polar_to_cartesian(1.0f, angle);
    //
    // // Flip y-axis because of screen coordinates, offset is inverted as well
    // Vector2f y = mult(-1.0f, perp(x));
    // offset.y = tile_height - offset.y;
    //
    // float accumulated_width = 0.0f;
    // float current_tile_width = (tile_width - offset.x) * scale.x;
    // src.x = offset.x * PIXELS_PER_UNIT;
    //
    // while (accumulated_width < width) {
    //     if ((width - accumulated_width) * scale.x < current_tile_width) {
    //         current_tile_width = (width - accumulated_width) * scale.x;
    //     }
    //
    //     float accumulated_height = 0.0f;
    //     float current_tile_height = (tile_height - offset.y) * scale.y;
    //     src.y = offset.y * PIXELS_PER_UNIT;
    //     while (accumulated_height < height) {
    //         if ((height - accumulated_height) * scale.y < current_tile_height) {
    //             current_tile_height = (height - accumulated_height) * scale.y;
    //         }
    //
    //         Vector2f p = sum(position, lin_comb(accumulated_width - 0.5f * (width - current_tile_width), x,
    //                                             accumulated_height - 0.5f * (height - current_tile_height), y));
    //         Vector2f pos = world_to_screen(camera, p);
    //
    //         src.w = current_tile_width * PIXELS_PER_UNIT / scale.x;
    //         src.h = current_tile_height * PIXELS_PER_UNIT / scale.y;
    //
    //         dest.w = current_tile_width * cam->zoom;
    //         dest.h = current_tile_height * cam->zoom;
    //         dest.x = pos.x - 0.5f * dest.w;
    //         dest.y = pos.y - 0.5f * dest.h;
    //
    //         SDL_RenderCopyExF(app.renderer, texture, &src, &dest, -to_degrees(angle), NULL, SDL_FLIP_NONE);
    //         // draw_rectangle_outline(camera, p, current_tile_width, current_tile_height, angle, 0.05f, COLOR_WHITE);
    //
    //         accumulated_height += current_tile_height;
    //         current_tile_height = tile_height * scale.y;
    //         src.y = 0;
    //     }
    //     accumulated_width += current_tile_width;
    //     current_tile_width = tile_width * scale.x;
    //     src.x = 0;
    // }
}


void draw_sprite_outline(int camera, int texture_index, float width, float height, int offset, Vector2f position,
        float angle, Vector2f scale) {
    if (texture_index == -1) {
        return;
    }

    // CameraComponent* cam = CameraComponent_get(camera);
    //
    // Vector2f scaling = mult(cam->zoom / PIXELS_PER_UNIT, scale);
    //
    // SDL_Texture* texture = resources.outline_textures[texture_index];
    //
    // SDL_Rect src = { 0, 0, width * PIXELS_PER_UNIT, height * PIXELS_PER_UNIT };
    // if (offset != 0) {
    //     src.x = offset * width * PIXELS_PER_UNIT;
    // }
    //
    // bool tile = false;
    // int texture_width;
    // int texture_height;
    // SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);
    // if (texture_width < width * PIXELS_PER_UNIT || texture_height < height * PIXELS_PER_UNIT) {
    //     tile = true;
    // }
    //
    // if (tile) {
    //     draw_rectangle_outline(camera, position, width, height, angle, 0.05f, COLOR_WHITE);
    // } else {
    //     if (width == 0.0f || height == 0.0f) {
    //         SDL_QueryTexture(texture, NULL, NULL, &src.w, &src.h);
    //         width = (float)src.w / PIXELS_PER_UNIT;
    //         height = (float)src.h / PIXELS_PER_UNIT;
    //     }
    //
    //     Vector2f pos = world_to_screen(camera, position);
    //
    //     SDL_FRect dest;
    //     dest.w = width * scaling.x * PIXELS_PER_UNIT;
    //     dest.h = height * scaling.y * PIXELS_PER_UNIT;
    //     dest.x = pos.x - 0.5f * dest.w;
    //     dest.y = pos.y - 0.5f * dest.h;
    //
    //     SDL_Rect* psrc = &src;
    //     if (width == 0.0f || height == 0.0f) {
    //         SDL_Rect* psrc = NULL;
    //     }
    //     SDL_RenderCopyExF(app.renderer, texture, psrc, &dest, -to_degrees(angle), NULL, SDL_FLIP_NONE);
    // }
}


void draw_text(int camera, Vector2f position, String string, int size, Color color) {
    if (color.a == 0) {
        return;
    }

    if (string[0] == '\0') {
        return;
    }

    CameraComponent* cam = CameraComponent_get(camera);
    int font_size = size * cam->zoom / 40.0f;
    TTF_Font* font = resources.fonts[font_size];
    if (!font) {
        return;
    }

    Vector2f pos = world_to_screen(camera, position);
    SDL_Color c = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Blended(font, string, 0, c);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app.renderer, surface);
    SDL_FRect dest = { pos.x - 0.5f * surface->w, pos.y - 0.5f * surface->h, surface->w, surface->h };
    LOG_INFO("surface width: %d, height: %d", surface->w, surface->h);
    SDL_RenderTexture(app.renderer, texture, NULL, &dest);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
}


void draw_skewed(int camera, int texture_index, Vector2f corners[4]) {
    SDL_Vertex vertices[4];
    for (int i = 0; i < 4; i++) {
        Vector2f v = world_to_screen(camera, corners[i]);
        vertices[i].position = (SDL_FPoint) { v.x, v.y };
        vertices[i].color = color_to_fcolor((Color) { 255, 255, 255, 255 });
    }
    vertices[0].tex_coord = (SDL_FPoint) { 0.0f, 0.0f };
    vertices[1].tex_coord = (SDL_FPoint) { 1.0f, 0.0f };
    vertices[2].tex_coord = (SDL_FPoint) { 1.0f, 1.0f };
    vertices[3].tex_coord = (SDL_FPoint) { 0.0f, 1.0f };

    int indices[6] = { 0, 1, 2, 2, 3, 0 };

    SDL_Texture* texture = resources.textures[texture_index];
    int err = SDL_RenderGeometry(app.renderer, texture, vertices, 4, indices, 6);
    if (err != 0) {
        LOG_ERROR("Error drawing skewed texture: %s", SDL_GetError());
    } 
}


void draw_spline(Entity camera, int texture_index, Vector2f p0, Vector2f p1, Vector2f p2, Vector2f p3, float width, 
        bool flip, Color color) {
    // https://www.mvps.org/directx/articles/catmull/
    static Matrix4 CATMULL_ROM = { 
        0.0f, 1.0f, 0.0f, 0.0f,
        -0.5f, 0.0f, 0.5f, 0.0f,
        1.0f, -2.5f, 2.0f, -0.5f,
        -0.5f, 1.5f, -1.5f, 0.5f
    };

    int points_size = 10;

    int vertices_size = 2 * points_size;
    SDL_Vertex* vertices = malloc(vertices_size * sizeof(SDL_Vertex));

    Vector4 x = { p0.x, p1.x, p2.x, p3.x };
    Vector4 y = { p0.y, p1.y, p2.y, p3.y };

    Vector4 mx = matrix4_map(CATMULL_ROM, x);
    Vector4 my = matrix4_map(CATMULL_ROM, y);

    for (int i = 0; i < points_size; i++) {
        float t = i / (float) (points_size - 1);

        Vector4 ts = { 1.0f, t, t * t, t * t * t };
        Vector2f pos = { dot4(ts, mx), dot4(ts, my) };

        Vector4 dts = { 0.0f, 1.0f, 2.0f * t, 3.0f * t * t };
        Vector2f dir = { dot4(dts, mx), dot4(dts, my) };
        Vector2f normal = mult(0.5f * width, normalized(perp(dir)));

        if (flip) {
            t = 1.0f - t;
        }

        Vector2f v = world_to_screen(camera, sum(pos, normal));
        vertices[2 * i].position = (SDL_FPoint) { v.x, v.y };
        vertices[2 * i].color = color_to_fcolor(color);
        vertices[2 * i].tex_coord = (SDL_FPoint) { t, 0.0f };

        v = world_to_screen(camera, diff(pos, normal));
        vertices[2 * i + 1].position = (SDL_FPoint) { v.x, v.y };
        vertices[2 * i + 1].color = color_to_fcolor(color);
        vertices[2 * i + 1].tex_coord = (SDL_FPoint) { t, 1.0f };
    }

    draw_triangle_strip(camera, texture_index, vertices, vertices_size);

    free(vertices);
}


void update_camera(int camera, float time_step, bool follow_players) {
    CoordinateComponent* coord = CoordinateComponent_get(camera);
    CameraComponent* cam = CameraComponent_get(camera);

    Vector2f pos = zeros();

    if (follow_players) {
        int n = 0;
        ListNode* node;
        FOREACH (node, game_data->components->player.order) {
            int i = node->value;
            PlayerComponent* player = PlayerComponent_get(i);
            if (player->state != PLAYER_DEAD) {
                n += 1;
                pos = sum(pos, get_position(i));
            }
        }
        if (n != 0) {
            pos = mult(1.0 / n, pos);
            coord->position = sum(coord->position, mult(10.0 * time_step, diff(pos, coord->position)));
        }
    }

    cam->zoom += 10.0 * time_step * (cam->zoom_target * cam->resolution.h / 720.0 - cam->zoom);
    cam->shake.velocity = diff(cam->shake.velocity, lin_comb(5.0f, cam->shake.position, 0.1f, cam->shake.velocity));
    cam->shake.position = sum(cam->shake.position, mult(time_step, cam->shake.velocity));
}


bool on_screen(int camera, Vector2f position, float width, float height) {
    Vector2f pos = get_position(camera);
    CameraComponent* cam = CameraComponent_get(camera);

    if (fabsf(position.x - pos.x) < 0.5f * (width + cam->resolution.w / cam->zoom)) {
        if (fabsf(position.y - pos.y) < 0.5f * (height + cam->resolution.h / cam->zoom)) {
            return true;
        }
    }
    return false;
}


void shake_camera(float speed) {
    for (int i = 0; i < game_data->components->entities; i++) {
        CameraComponent* cam = CameraComponent_get(i);
        if (cam) {
            cam->shake.velocity = sum(cam->shake.velocity, mult(speed, rand_vector()));
        }
    }
}


void draw_overlay(int camera, float alpha) {
    CameraComponent* cam = CameraComponent_get(camera);
    float width = cam->resolution.w / cam->zoom;
    float height = cam->resolution.h / cam->zoom;
    Vector2f pos = get_position(camera);
    Color color = get_color(0.0f, 0.0f, 0.0f, alpha);
    draw_rectangle(camera, pos, width, height, 0.0f, color);
}
