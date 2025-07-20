#pragma once


typedef struct MeshParameters {
    String mesh_filename;
    String texture_filename;
    String material_filename;
    Visibility visibility;
} MeshParameters;


typedef struct MeshComponent {
    int mesh_index;
    int texture_index;
    int material_index;
    Visibility visibility;
} MeshComponent;


MeshComponent* MeshComponent_add(int entity, MeshParameters params);

void MeshComponent_remove(int entity);
