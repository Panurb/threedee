#include <stdio.h>

#include "component.h"
#include "components/mesh.h"
#include "resources.h"
#include "scene.h"


MeshComponent* MeshComponent_add(Entity entity, MeshParameters params) {
    MeshComponent* mesh = malloc(sizeof(MeshComponent));
    mesh->mesh_index = -1;
    mesh->texture_index = -1;
    mesh->material_index = -1;
    mesh->visibility = params.visibility ? params.visibility : LIGHT_ALL;
    mesh->texture_scale = ones2();
    mesh->hidden = false;

    if (strcmp(params.mesh_filename, "cube") == 0) {
        mesh->texture_scale = zeros2();
    }

    if (params.mesh_filename[0] != '\0') {
        mesh->mesh_index = binary_search_filename(params.mesh_filename, resources.mesh_names, resources.meshes_size);
        if (mesh->mesh_index == -1) {
            LOG_ERROR("Mesh not found: %s", params.mesh_filename);
            free(mesh);
            return NULL;
        }
    }

    if (params.texture_filename[0] != '\0') {
        mesh->texture_index = binary_search_filename(params.texture_filename, resources.texture_names, resources.textures_size);
        if (mesh->texture_index == -1) {
            LOG_ERROR("Texture not found: %s", params.texture_filename);
        }
    }

    if (params.material_filename[0] != '\0') {
        mesh->material_index = binary_search_filename(params.material_filename, resources.material_names, resources.materials_size);
        if (mesh->material_index == -1) {
            LOG_ERROR("Material not found: %s", params.material_filename);
        }
    } else {
        mesh->material_index = binary_search_filename("default", resources.material_names, resources.materials_size);
    }

    scene->components->mesh[entity] = mesh;
    return mesh;
}


void MeshComponent_remove(Entity entity) {
    MeshComponent* mesh = scene->components->mesh[entity];
    if (mesh) {
        free(mesh);
        scene->components->mesh[entity] = NULL;
    }
}
