#pragma once


#define FRAME_WINDOW 100


typedef struct {
    float frame_times[FRAME_WINDOW];
    float frame_avg;
    int iterator;
} FpsCounter;

FpsCounter* FpsCounter_create();

void FPSCounter_update(FpsCounter* fps, float delta_time);

void FPSCounter_draw(FpsCounter* counter);
