#include "components/camera.h"
#include "scene.h"


CameraComponent* CameraComponent_add(Entity entity, Resolution resolution, float fov) {
    CameraComponent* camera = malloc(sizeof(CameraComponent));

    camera->resolution = resolution;
    camera->fov = fov;
    camera->near_plane = 0.1f;
    camera->far_plane = 100.0f;

    float aspect_ratio = (float)camera->resolution.w / (float)camera->resolution.h;
    camera->projection_matrix = perspective_projection_matrix(
        camera->fov, aspect_ratio, camera->near_plane, camera->far_plane
    );

    scene->components->camera[entity] = camera;
    return camera;
}


void CameraComponent_remove(Entity entity) {
    CameraComponent* camera = get_component(entity, COMPONENT_CAMERA);
    if (camera) {
        free(camera);
        scene->components->camera[entity] = NULL;
    }
}
