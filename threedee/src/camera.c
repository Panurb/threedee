#include "render.h"
#include "camera.h"

#include <stdio.h>

#include "component.h"
#include "util.h"
#include "settings.h"


SDL_FColor color_to_fcolor(Color color) {
    return (SDL_FColor) { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}


int create_camera() {
    int i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, 1.0f, 0.0f),
    });
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, to_radians(game_settings.fov));
    return i;
}


int create_screen_camera() {
    int i = create_entity();
    TransformComponent_add(i, (TransformParameters) {});
    CameraComponent* cam = CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, 25.0f);
    float aspect_ratio = (float) cam->resolution.w / (float) cam->resolution.h;
    cam->projection_matrix = orthographic_projection_matrix(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);
    return i;
}


Entity create_overhead_camera() {
    Entity i = create_entity();
    TransformComponent_add(i, (TransformParameters) {
        .position = vec3(0.0f, 10.0f, 0.0f),
    });
    look_at(i, vec3(0.0f, 0.0f, 0.0f));
    CameraComponent_add(i, (Resolution) { game_settings.width, game_settings.height }, to_radians(90.0f));
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
    forward = map4(transform, forward);
    return normalized3(vec3(forward.x, forward.y, forward.z));
}


Vector3 world_to_screen(int camera, Vector3 a) {
    // Matrix4 transform = TransformComponent_get(camera)->transform;

    return a;
}


Vector3 screen_to_world(int camera, Vector3 a) {
    return a;
}


Frustum get_camera_frustum(Entity entity) {
    Axes axes = get_axes(entity);
    Matrix4 transform = get_transform(entity);
    Vector3 position = position_from_transform(transform);

    CameraComponent* cam = get_component(entity, COMPONENT_CAMERA);
    Frustum frustum = {
        .origin =  position,
    };

    Vector3 far_center = add3(position, mul3(cam->far_plane, axes.forward));
    float w = cam->far_plane * tanf(cam->fov / 2.0f);
    float h = w / cam->aspect_ratio;

    Vector3 tl = lin_comb3(1.0f, far_center, -w, axes.right,  h, axes.up);
    Vector3 tr = lin_comb3(1.0f, far_center,  w, axes.right,  h, axes.up);
    Vector3 bl = lin_comb3(1.0f, far_center, -w, axes.right, -h, axes.up);
    Vector3 br = lin_comb3(1.0f, far_center,  w, axes.right, -h, axes.up);

    frustum.corners[0] = tl;
    frustum.corners[1] = tr;
    frustum.corners[2] = br;
    frustum.corners[3] = bl;

    Vector3 left_normal = normalized3(cross(sub3(bl, tl), sub3(tl, position)));
    Vector3 right_normal = normalized3(cross(sub3(tr, br), sub3(br, position)));
    Vector3 top_normal = normalized3(cross(sub3(tl, tr), sub3(tr, position)));
    Vector3 bottom_normal = normalized3(cross(sub3(br, bl), sub3(bl, position)));

    frustum.far_plane.normal = axes.back;
    frustum.far_plane.offset = dot3(axes.back, far_center);

    frustum.near_plane.normal = axes.forward;
    frustum.near_plane.offset = dot3(axes.forward, position) + cam->near_plane;

    frustum.left_plane.normal = left_normal;
    frustum.left_plane.offset = dot3(left_normal, tl);

    frustum.right_plane.normal = right_normal;
    frustum.right_plane.offset = dot3(right_normal, tr);

    frustum.top_plane.normal = top_normal;
    frustum.top_plane.offset = dot3(top_normal, tr);

    frustum.bottom_plane.normal = bottom_normal;
    frustum.bottom_plane.offset = dot3(bottom_normal, bl);

    return frustum;
}
