#include <stdio.h>

#include "interface.h"
#include "render.h"
#include "camera.h"
#include "scene.h"


FpsCounter* FpsCounter_create() {
    FpsCounter* fps = malloc(sizeof(FpsCounter));
    for (int i = 0; i < FRAME_WINDOW; i++) {
        fps->frame_times[i] = 1.0 / 60.0;
    }
    fps->frame_avg = 1.0 / 60.0;
    fps->iterator = 0;

    return fps;
}


void FPSCounter_update(FpsCounter* fps, float delta_time) {
    fps->frame_avg -= fps->frame_times[fps->iterator] / FRAME_WINDOW;
    fps->frame_times[fps->iterator] = delta_time;
    fps->frame_avg += fps->frame_times[fps->iterator] / FRAME_WINDOW;

    fps->iterator = (fps->iterator + 1) % FRAME_WINDOW;
}


void FPSCounter_draw(FpsCounter* fps) {
    char buffer[20];
    snprintf(buffer, 20, "%.0f", 1.0 / fps->frame_avg);

    Vector2 size = camera_size(scene->screen_camera);
    Vector2 pos = vec2(-0.49f * size.x, 0.49f * size.y);
    draw_text(buffer, pos, 0.0f, 0.02f, COLOR_WHITE);
}
