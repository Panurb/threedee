#pragma once


typedef enum {
    DEVICE_UNBOUND,
    DEVICE_KEYBOARD,
    DEVICE_MOUSE
} InputDevice;

typedef struct {
    InputDevice device;
    int key;
} Keybind;

typedef enum {
    QUALITY_LOW,
    QUALITY_MEDIUM,
    QUALITY_HIGH
} Quality;

typedef struct {
    int width;
    int height;
    int antialiasing;
    bool fullscreen;
    bool vsync;
    int max_fps;
    int volume;
    int music;
    Quality particles;
    bool debug;
    Keybind keybinds[16];
} Settings;

typedef struct {
    char* key;
    char* value;
} KeyValue;

typedef char Line[255];

extern Settings game_settings;

extern int keyboard_controls[12];

KeyValue parse_line(Line string, char* delim);

void load_settings();

void save_settings();
