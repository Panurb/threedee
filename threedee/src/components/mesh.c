#include "component.h"
#include "components/mesh.h"
#include "scene.h"
#include "render.h"


MeshComponent* MeshComponent_add(Entity entity, String mesh_filename) {
    MeshComponent* mesh = malloc(sizeof(MeshComponent));
    mesh->mesh_index = -1;
    mesh->texture_index = -1;

    if (strcmp(mesh_filename, "cube") == 0) {
        mesh->mesh_index = MESH_CUBE;
    } else if (strcmp(mesh_filename, "quad") == 0) {
        mesh->mesh_index = MESH_QUAD;
    } else if (strcmp(mesh_filename, "cube_textured") == 0) {
        mesh->mesh_index = MESH_CUBE_TEXTURED;
    } else {
        LOG_ERROR("Unknown mesh filename: %s", mesh_filename);
        free(mesh);
        return NULL;
    }

    scene->components->mesh[entity] = mesh;
    return mesh;
}


void MeshComponent_remove(int entity) {
    MeshComponent* mesh = scene->components->mesh[entity];
    if (mesh) {
        free(mesh);
        scene->components->mesh[entity] = NULL;
    }
}
