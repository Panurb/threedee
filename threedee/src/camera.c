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
    TransformComponent_add(i, vec3(0.0f, 1.0f, 0.0f));
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, to_radians(game_settings.fov));
    return i;
}


int create_menu_camera() {
    int i = create_entity();
    TransformComponent_add(i, zeros3());
    CameraComponent* cam = CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, 25.0f);
    float aspect_ratio = (float) cam->resolution.w / (float) cam->resolution.h;
    cam->projection_matrix = orthographic_projection_matrix(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    return i;
}


Vector2 camera_size(int camera) {
    CameraComponent* cam = get_component(camera, COMPONENT_CAMERA);
    float aspect_ratio = cam->resolution.w / (float) cam->resolution.h;
    return (Vector2) { 2.0f * aspect_ratio, 2.0f };
}


Vector3 look_direction(Entity camera) {
    Matrix4 transform = get_transform(camera);
    Vector4 forward = vec4(0.0f, 0.0f, -1.0f, 0.0f);
    forward = matrix4_map(transform, forward);
    return vec3(forward.x, forward.y, forward.z);
}


Vector3 world_to_screen(int camera, Vector3 a) {
    // Matrix4 transform = TransformComponent_get(camera)->transform;

    return a;
}


Vector3 screen_to_world(int camera, Vector3 a) {
    return a;
}
