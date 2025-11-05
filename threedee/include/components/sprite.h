#pragma once


typedef struct SpriteComponent {
    float width;
    float height;
    Vector2 uv_top_left;
    Vector2 uv_bottom_right;
    int texture_index;
} SpriteComponent;


typedef struct SpriteParameters {
    float width;
    float height;
    Vector2 uv_top_left;
    Vector2 uv_bottom_right;
    String texture_filename;
} SpriteParameters;


SpriteComponent* SpriteComponent_add(Entity entity, SpriteParameters params);

void SpriteComponent_remove(Entity entity);
