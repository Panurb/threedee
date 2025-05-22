#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL3/SDL.h>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "app.h"
#include "game.h"
#include "settings.h"
#include "util.h"


static float elapsed_time = 0.0f;
static float time_since_last_update = 0.0f;


float get_delta_time() {
    static int start_time = 0;
    if (start_time == 0) {
        start_time = SDL_GetTicks();
    }
    int current_time = SDL_GetTicks();
    float delta_time = (current_time - start_time) / 1000.0f;
    start_time = current_time;
    return delta_time;
}


void main_loop() {
    float delta_time = get_delta_time();
    time_since_last_update += delta_time;

    input();
    LOG_INFO("Inputed");

    if (app.focus) {
        while (elapsed_time > app.time_step) {
            elapsed_time -= app.time_step;
            time_since_last_update = 0.0f;
            update(app.time_step);
        }

        elapsed_time += delta_time;
        FPSCounter_update(app.fps, delta_time);
    }
    
    // 0 means only interpolate, 1 means only extrapolate
    float extrapolation_factor = 0.5f;

    app.delta = fminf(time_since_last_update / app.time_step, 1.0f) + extrapolation_factor;

    draw();
    play_audio();
}


int main(int argc, char* argv[]) {
    load_settings();

    init();

    load_resources();
    create_game();

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(main_loop, 0, true);
        LOG_INFO("Emscripten main loop started");
    #else
        while (!app.quit) {
            main_loop();
        }
    #endif

    quit();

    return 0;
}
