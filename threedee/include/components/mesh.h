#pragma once


typedef struct MeshComponent {
    int mesh_index;
    int texture_index;
    int material_index;
    LightType visibility;
} MeshComponent;


MeshComponent* MeshComponent_add(int entity, String mesh_filename, String texture_filename, String material_filename);

void MeshComponent_remove(int entity);
