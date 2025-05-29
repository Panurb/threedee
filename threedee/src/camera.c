#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "app.h"
#include "camera.h"

#include <render.h>

#include "component.h"
#include "util.h"
#include "settings.h"


SDL_FColor color_to_fcolor(Color color) {
    return (SDL_FColor) { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}


int create_camera() {
    int i = create_entity();
    TransformComponent_add(i, zeros3(), zeros3());
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, M_PI_2);
    return i;
}


int create_menu_camera() {
    int i = create_entity();
    TransformComponent_add(i, zeros3(), zeros3());
    CameraComponent* cam = CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, 25.0f);
    float aspect_ratio = (float) cam->resolution.w / (float) cam->resolution.h;
    cam->projection_matrix = orthographic_projection_matrix(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    return i;
}


Vector2 camera_size(int camera) {
    CameraComponent* cam = CameraComponent_get(camera);
    float aspect_ratio = cam->resolution.w / (float) cam->resolution.h;
    return (Vector2) { 2.0f * aspect_ratio, 2.0f };
}


Vector3 world_to_screen(int camera, Vector3 a) {
    // Matrix4 transform = TransformComponent_get(camera)->transform;

    return a;
}


Vector3 screen_to_world(int camera, Vector3 a) {
    return a;
}


void draw_cube(Entity camera, Vector3 position, float size, Color color) {
    CameraComponent* cam = CameraComponent_get(camera);

    Matrix4 object_transform = transform_matrix(position, zeros3(), diag3(size));

    add_render_instance(RENDER_CUBE, object_transform);
}


void draw_triangle_fan(int camera, SDL_Vertex* vertices, int verts_size) {

}


void draw_triangle_strip(int camera, int texture_index, SDL_Vertex* vertices, int verts_size) {

}


void draw_line(int camera, Vector2 start, Vector2 end, float width, Color color) {

}


void draw_circle(int camera, Vector2 position, float radius, Color color) {

}


void draw_circle_outline(int camera, Vector2 position, float radius, float line_width, Color color) {

}


void draw_ellipse(int camera, Vector2 position, float major, float minor, float angle, Color color) {

}


void draw_rectangle(int camera, Vector2 position, float width, float height, float angle, Color color) {

}



void draw_rectangle_outline(int camera, Vector2 position, float width, float height,
        float angle, float line_width, Color color) {
    Vector2 corners[4];
    get_rect_corners(position, angle, width, height, corners);

    for (int i = 0; i < 4; i++) {
        draw_line(camera, corners[i], corners[(i + 1) % 4], line_width, color);
    }
}


void draw_sprite(int camera, int texture_index, float width, float height, int offset, Vector2 position, float angle,
        Vector2 scale, float alpha) {
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


void draw_tiles(int camera, int texture_index, float width, float height, Vector2 offset, Vector2 position,
        float angle, Vector2 scale, float alpha) {
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


void draw_text(int camera, Vector2 position, String string, int size, Color color) {
    if (color.a == 0) {
        return;
    }

    if (string[0] == '\0') {
        return;
    }

}


void draw_spline(Entity camera, int texture_index, Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float width,
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
        Vector2 pos = { dot4(ts, mx), dot4(ts, my) };

        Vector4 dts = { 0.0f, 1.0f, 2.0f * t, 3.0f * t * t };
        Vector2 dir = { dot4(dts, mx), dot4(dts, my) };
        Vector2 normal = mult(0.5f * width, normalized2(perp(dir)));

        if (flip) {
            t = 1.0f - t;
        }

        // Vector2 v = world_to_screen(camera, sum(pos, normal));
        // vertices[2 * i].position = (SDL_FPoint) { v.x, v.y };
        // vertices[2 * i].color = color_to_fcolor(color);
        // vertices[2 * i].tex_coord = (SDL_FPoint) { t, 0.0f };
        //
        // v = world_to_screen(camera, diff(pos, normal));
        // vertices[2 * i + 1].position = (SDL_FPoint) { v.x, v.y };
        // vertices[2 * i + 1].color = color_to_fcolor(color);
        // vertices[2 * i + 1].tex_coord = (SDL_FPoint) { t, 1.0f };
    }

    draw_triangle_strip(camera, texture_index, vertices, vertices_size);

    free(vertices);
}


void update_camera(Entity camera, float time_step) {

}


bool on_screen(int camera, Vector2 position, float width, float height) {

}
