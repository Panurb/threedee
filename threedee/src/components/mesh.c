#include <stdio.h>

#include "component.h"
#include "components/mesh.h"

#include <resources.h>

#include "scene.h"


MeshComponent* MeshComponent_add(Entity entity, String mesh_filename, String texture_filename, String material_filename) {
    MeshComponent* mesh = malloc(sizeof(MeshComponent));
    mesh->mesh_index = -1;
    mesh->texture_index = -1;
    mesh->material_index = -1;

    if (mesh_filename[0] != '\0') {
        mesh->mesh_index = binary_search_filename(mesh_filename, resources.mesh_names, resources.meshes_size);
        if (mesh->mesh_index == -1) {
            LOG_ERROR("Mesh not found: %s", mesh_filename);
            free(mesh);
            return NULL;
        }
    }

    if (texture_filename[0] != '\0') {
        mesh->texture_index = binary_search_filename(texture_filename, resources.texture_names, resources.textures_size);
        if (mesh->texture_index == -1) {
            LOG_ERROR("Texture not found: %s", texture_filename);
        }
    }

    if (material_filename[0] != '\0') {
        mesh->material_index = binary_search_filename(material_filename, resources.material_names, resources.materials_size);
        if (mesh->material_index == -1) {
            LOG_ERROR("Material not found: %s", material_filename);
        }
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
