#include <stdio.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
    #define NOMINMAX
    #include <windows.h>
#endif
#include <stdlib.h>

#include "widget.h"
#include "component.h"
#include "camera.h"
#include "collider.h"
#include "input.h"
#include "game.h"


void bring_to_top(int entity) {
    // Breadth-first search
    List* queue = List_create();
    List_add(queue, entity);
    while (queue->size > 0) {
        int entity = List_pop(queue);
        List_remove(game_data->components->widget.order, entity);
        List_append(game_data->components->widget.order, entity);
        CoordinateComponent* coord = CoordinateComponent_get(entity);
        for (ListNode* node = coord->children->head; node; node = node->next) {
            int child = node->value;
            List_append(queue, child);
        }
    }
    List_delete(queue);
}


int create_window(Vector2 position, ButtonText text, int width, OnClick on_close) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, width * BUTTON_WIDTH, BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent_add(i, text, WIDGET_WINDOW);

    int j = create_button_small("X", vec(0.5 * (width * BUTTON_WIDTH - BUTTON_HEIGHT), 0.0f), on_close);
    add_child(i, j);

    return i;
}


int create_label(ButtonText text, Vector2 position) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, BUTTON_WIDTH, BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent_add(i, text, WIDGET_LABEL);

    return i;
}


int create_button(ButtonText text, Vector2 position, OnClick on_click) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, BUTTON_WIDTH, BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent_add(i, text, WIDGET_BUTTON)->on_click = on_click;

    return i;
}


int create_button_small(ButtonText text, Vector2 position, OnClick on_click) {
    int i = create_entity();
    CoordinateComponent_add(i, sum(position, vec(0.0f, 0.25f * BORDER_WIDTH)), 0.0f);
    float height = BUTTON_HEIGHT - 0.5f * BORDER_WIDTH;
    ColliderComponent_add_rectangle(i, BUTTON_HEIGHT, height, GROUP_WALLS)->enabled = false;
    WidgetComponent_add(i, text, WIDGET_BUTTON)->on_click = on_click;

    return i;
}


int create_container(Vector2 position, int width, int height) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, width * BUTTON_WIDTH, height * BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent* widget = WidgetComponent_add(i, "", WIDGET_CONTAINER);
    widget->type = WIDGET_CONTAINER;

    return i;
}


void increment_value(int entity, int direction) {
    WidgetComponent* button = WidgetComponent_get(entity);
    Vector2 v = vec(0.0f, direction * BUTTON_HEIGHT);
    button->value += direction;

    if (button->value > button->max_value) {
        if (button->cyclic) {
            button->value = button->min_value;
            v.y = button->max_value - button->min_value;
        } else {
            button->value = button->max_value;
            v.y = 0.0f;
        }
    }
    if (button->value < button->min_value) {
        if (button->cyclic) {
            button->value = button->min_value;
            v.y = button->min_value - button->max_value;
        } else {
            button->value = button->min_value;
            v.y = 0.0f;
        }
    }

    float height = ColliderComponent_get(entity)->height;
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        CoordinateComponent* coord_child = CoordinateComponent_get(node->value);
        coord_child->position = sum(coord_child->position, v);

        Vector2 pos = coord_child->position;
        WidgetComponent_get(node->value)->enabled = (pos.y > -0.5f * height && pos.y < 0.5f * height);
    }

    if (button->on_change) {
        button->on_change(entity, direction);
    }
}


void scroll_container(int entity, int delta) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    coord = CoordinateComponent_get(coord->parent);
    for (ListNode* node = coord->children->head; node; node = node->next) {
        int i = node->value;
        if (WidgetComponent_get(i)->type == WIDGET_CONTAINER) {
            increment_value(i, delta);
        }
    }
}


void add_widget_to_container(int container, int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(container);
    WidgetComponent* widget = WidgetComponent_get(container);
    ColliderComponent* collider = ColliderComponent_get(container);

    int columns = collider->width / BUTTON_WIDTH;
    int rows = coord->children->size / columns;
    Vector2 pos = vec(0.0f, 0.5f * collider->height - rows * BUTTON_HEIGHT - 0.5f * BUTTON_HEIGHT);
    CoordinateComponent* coord_child = CoordinateComponent_get(entity);
    coord_child->position = pos;
    add_child(container, entity);

    int n = columns * collider->height / BUTTON_HEIGHT;
    if (coord->children->size > n) {
        widget->max_value += 1;
        WidgetComponent_get(entity)->enabled = false;
    }
}


int add_button_to_container(int container, ButtonText string, OnClick on_click) {
    int i = create_button(string, zeros(), on_click);
    add_widget_to_container(container, i);
    return i;
}


void add_row_to_container(int container, int left, int right) {
    add_widget_to_container(container, left);
    CoordinateComponent* coord_left = CoordinateComponent_get(left);
    coord_left->position.x -= 0.5f * BUTTON_WIDTH;

    add_child(container, right);
    CoordinateComponent* coord_right = CoordinateComponent_get(right);
    Vector2 pos = sum(coord_left->position, vec(BUTTON_WIDTH, 0.0f));
    float height = ColliderComponent_get(container)->height;
    coord_right->position = pos;
    WidgetComponent_get(right)->enabled = (pos.y > -0.5f * height && pos.y < 0.5f * height);
}


void add_files_to_container(int container, Filename directory, OnClick on_click) {
    #ifndef __EMSCRIPTEN__
        Filename path;
        snprintf(path, 128, "%s/%s/*.*", "data", directory);

        WIN32_FIND_DATA file;
        HANDLE handle = FindFirstFile(path, &file);

        if (handle == INVALID_HANDLE_VALUE) {
            printf("Path not found: %s\n", path);
            return;
        }

        do {
            if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0) {
                continue;
            }
            char* dot = strchr(file.cFileName, '.');
            if (dot) {
                *dot = '\0';
            }
            add_button_to_container(container, file.cFileName, on_click);
        } while (FindNextFile(handle, &file));
    #endif
}


void close_dropdown(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    ListNode* node;
    FOREACH (node, coord->children) {
        int i = node->value;
        destroy_entity_recursive(i);
    }
    List_clear(coord->children);
}


void set_dropdown(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    int dropdown = CoordinateComponent_get(coord->parent)->parent;
    WidgetComponent* widget = WidgetComponent_get(entity);
    WidgetComponent* widget_dropdown = WidgetComponent_get(dropdown);
    widget_dropdown->value = widget->value;

    if (widget_dropdown->on_change) {
        widget_dropdown->on_change(dropdown, widget->value);
    }

    close_dropdown(dropdown);
}


void update_scrollbar(int entity, int delta) {
    UNUSED(delta);
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    coord = CoordinateComponent_get(coord->parent);
    int value = WidgetComponent_get(entity)->value;
    for (ListNode* node = coord->children->head; node; node = node->next) {
        int i = node->value;
        WidgetComponent* widget = WidgetComponent_get(i);
        if (widget->type == WIDGET_SCROLLBAR) {
            widget->value = value;
        }
    }
}


void add_scrollbar_to_container(int container) {
    CoordinateComponent* coord = CoordinateComponent_get(container);
    WidgetComponent* widget = WidgetComponent_get(container);
    ColliderComponent* collider = ColliderComponent_get(container);

    widget->on_change = update_scrollbar;

    for (ListNode* node = coord->children->head; node; node = node->next) {
        int i = node->value;
        ColliderComponent_get(i)->width = BUTTON_WIDTH - SCROLLBAR_WIDTH;
        CoordinateComponent* coord_child = CoordinateComponent_get(i);
        coord_child->position = diff(coord_child->position, vec(0.5f * SCROLLBAR_WIDTH, 0.0f));
    }

    int parent = coord->parent;
    int height = collider->height / BUTTON_HEIGHT;
    float w = ColliderComponent_get(parent)->width;
    Vector2 pos = vec(0.5f * w - 0.5f * SCROLLBAR_WIDTH, -0.5f * (height + 1) * BUTTON_HEIGHT);
    int scrollbar = create_scrollbar(pos, height, widget->max_value, scroll_container);
    add_child(parent, scrollbar);
}


void toggle_dropdown(int entity) {
    CoordinateComponent* coord = CoordinateComponent_get(entity);
    WidgetComponent* widget = WidgetComponent_get(entity);
    if (coord->children->size > 0) {
        close_dropdown(entity);
    } else {
        int height = mini(3, widget->max_value + 1);

        int container = create_container(vec(0.0f, -0.5f * (height + 1) * BUTTON_HEIGHT), 1, height);
        add_child(entity, container);
        for (int i = widget->min_value; i <= widget->max_value; i++) {
            int j = add_button_to_container(container, "", set_dropdown);
            WidgetComponent* widget_child = WidgetComponent_get(j);
            widget_child->value = i;
            widget_child->strings = widget->strings;
        }

        if (widget->max_value > 3) {
            add_scrollbar_to_container(container);
        }
    }
}


int create_dropdown(Vector2 position, ButtonText* strings, int size) {
    int i = create_button(strings[0], position, toggle_dropdown);
    WidgetComponent* widget = WidgetComponent_get(i);
    widget->strings = strings;
    widget->max_value = size - 1;
    widget->type = WIDGET_DROPDOWN;

    return i;
}


void set_slider(int entity, Vector2 mouse_position) {
    WidgetComponent* widget = WidgetComponent_get(entity);
    ColliderComponent* collider = ColliderComponent_get(entity);
    float x = mouse_position.x - get_position(entity).x + 0.5f * collider->width;
    int n = widget->max_value - widget->min_value;
    int value = clamp(x  / collider->width, 0.0f, 1.0f) * n;
    widget->value = value;
    if (widget->on_change) {
        widget->on_change(entity, value);
    }
}


int create_slider(Vector2 position, int min_value, int max_value, int value,
        OnChange on_change) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, BUTTON_WIDTH, BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent* widget = WidgetComponent_add(i, "", WIDGET_SLIDER);
    widget->min_value = min_value;
    widget->max_value = max_value;
    widget->value = value;
    widget->on_change = on_change;

    return i;
}


void set_scrollbar(int entity, Vector2 mouse_position) {
    WidgetComponent* widget = WidgetComponent_get(entity);
    ColliderComponent* collider = ColliderComponent_get(entity);
    float y = mouse_position.y - get_position(entity).y + 0.5f * collider->height;
    int n = widget->max_value - widget->min_value;
    int value = clamp(1.0f - y  / collider->height, 0.0f, 0.99f) * (n + 1);
    int delta = value - widget->value;
    widget->value = value;
    if (widget->on_change) {
        widget->on_change(entity, delta);
    }
}


int create_scrollbar(Vector2 position, int height, int max_value, OnChange on_change) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent* collider = ColliderComponent_add_rectangle(i, SCROLLBAR_WIDTH, 
        BUTTON_HEIGHT * height, GROUP_WALLS);
    collider->enabled = false;
    WidgetComponent* widget = WidgetComponent_add(i, "", WIDGET_SCROLLBAR);
    widget->max_value = max_value;
    widget->on_change = on_change;

    return i;
}


int create_textbox(Vector2 position, int width) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent* collider = ColliderComponent_add_rectangle(i, BUTTON_WIDTH * width, 
        BUTTON_HEIGHT, GROUP_WALLS);
    collider->enabled = false;
    WidgetComponent* widget = WidgetComponent_add(i, "", WIDGET_TEXTBOX);
    widget->max_value = width * 18;

    return i;
}


void toggle_checkbox(int entity) {
    WidgetComponent* widget = WidgetComponent_get(entity);
    widget->value = !widget->value;
    if (widget->on_change) {
        widget->on_change(entity, widget->value);
    }
}


int create_checkbox(Vector2 position, bool value, OnChange on_change) {
    int i = create_entity();
    CoordinateComponent_add(i, position, 0.0f);
    ColliderComponent_add_rectangle(i, BUTTON_WIDTH, BUTTON_HEIGHT, GROUP_WALLS)->enabled = false;
    WidgetComponent* widget = WidgetComponent_add(i, "", WIDGET_CHECKBOX);
    widget->value = value;
    widget->on_click = toggle_checkbox;

    return i;
}


void draw_checkbox(int camera, Vector2 position, bool value, bool selected) {
    float height = 0.8f * BUTTON_HEIGHT;
    Color color = selected ? COLOR_SELECTED : COLOR_BUTTON;
    draw_rectangle(camera, position, height, height, 0.0f, color);
    if (value) {
        draw_text(camera, position, "X", 40, COLOR_TEXT);
    }
}


void update_widgets(int camera) {
    int last_selected = -1;
    ListNode* node;
    FOREACH(node, game_data->components->widget.order) {
        int i = node->value;
        WidgetComponent* widget = WidgetComponent_get(i);
        widget->selected = false;

        if (!widget->enabled) continue;

        Vector2 mouse = get_mouse_position(camera);
        if (point_inside_collider(i, mouse)) {
            last_selected = i;
        }
    }
    if (last_selected != -1) {
        WidgetComponent_get(last_selected)->selected = true;
    }
}


void draw_button(int camera, Vector2 position, float width,
        float height, bool selected) {
    draw_rectangle(camera, position, width, height, 0.0f, COLOR_SHADOW);
    Vector2 r = sum(position, vec(-0.5f * BORDER_WIDTH, 0.5f * BORDER_WIDTH));
    draw_rectangle(camera, r, width - BORDER_WIDTH, height - BORDER_WIDTH, 0.0f, 
        COLOR_SELECTED);
    Color color = selected ? COLOR_SELECTED : COLOR_BUTTON;
    draw_rectangle(camera, position, width - 2.0f * BORDER_WIDTH, 
        height - 2.0f * BORDER_WIDTH, 0.0f, color);
}


void draw_widgets(int camera) {
    ListNode* node;
    FOREACH(node, game_data->components->widget.order) {
        int i = node->value;
        WidgetComponent* widget = WidgetComponent_get(i);
        if (!widget->enabled) continue;

        Vector2 pos = get_position(i);
        CoordinateComponent* coord = CoordinateComponent_get(i);
        ColliderComponent* collider = ColliderComponent_get(i);

        float w = collider->width;
        float h = collider->height;
        Vector2 r = zeros();
        switch (widget->type) {
        case WIDGET_WINDOW:
            draw_rectangle(camera, pos, w + BORDER_WIDTH, h + BORDER_WIDTH, 0.0f, 
                COLOR_BORDER);
            draw_rectangle(camera, pos, w, h, 0.0f, COLOR_SHADOW);
            break;
        case WIDGET_CONTAINER:
            draw_rectangle(camera, pos, w + BORDER_WIDTH, h + BORDER_WIDTH, 0.0f, 
                COLOR_BORDER);
            draw_rectangle(camera, pos, w, h, 0.0f, COLOR_CONTAINER);
            if (coord->children->size > h / BUTTON_HEIGHT) {
                r = sum(pos, vec(0.5f * w - 0.5f * BUTTON_HEIGHT, 0.0f));
                draw_rectangle(camera, r, BUTTON_HEIGHT, h, 0.0f, COLOR_CONTAINER);
            }
            break;
        case WIDGET_LABEL:
            break;
        case WIDGET_BUTTON:
            draw_button(camera, pos, w - MARGIN, h - MARGIN, widget->selected);
            break;
        case WIDGET_DROPDOWN:
            draw_rectangle(camera, pos, w - MARGIN, h - MARGIN, 0.0f, COLOR_SHADOW);

            r = sum(pos, vec(-0.1f, 0.1f));
            draw_rectangle(camera, r, w - BORDER_WIDTH - MARGIN, h - BORDER_WIDTH - MARGIN, 0.0f, COLOR_BUTTON);

            r = sum(pos, vec(0.5f * BUTTON_WIDTH - 0.5f * BUTTON_HEIGHT, 0.0f));
            if (CoordinateComponent_get(i)->children->size == 0) {
                draw_text(camera, r, "v", 20, COLOR_WHITE);
            } else {
                draw_text(camera, r, "^", 20, COLOR_WHITE);
            }
            break;
        case WIDGET_SLIDER:
            draw_rectangle(camera, pos, w - MARGIN, h - MARGIN, 0.0f, COLOR_SHADOW);
            float x = widget->value / (float) (widget->max_value - widget->min_value);
            draw_rectangle(camera, sum(pos, vec(0.5f * (x - 1.0f) * w, 0.0f)), x * w, h, 
                0.0f, COLOR_BUTTON);
            char buffer[256];
            sprintf(buffer, "%d", widget->value);
            draw_text(camera, pos, buffer, 20, COLOR_TEXT);
            break;
        case WIDGET_SCROLLBAR:
            draw_rectangle(camera, pos, w, h, 0.0f, COLOR_CONTAINER);
            float n = widget->max_value - widget->min_value + 1;
            float y = widget->value / n;
            float bar = h / n;
            r = sum(pos, vec(0.0f, 0.5f * (h - bar) - y * h));
            draw_button(camera, r, w, bar, widget->selected);
            break;
        case WIDGET_TEXTBOX:
            draw_rectangle(camera, pos, w, h, 0.0f, COLOR_SHADOW);
            draw_text(camera, pos, widget->string, 20, COLOR_WHITE);
            break;
        case WIDGET_CHECKBOX:
            draw_checkbox(camera, pos, widget->value, widget->selected);
            break;
        default:
            draw_rectangle(camera, pos, w, h, 0.0f, COLOR_SHADOW);
        }

        if (widget->strings) {
            draw_text(camera, pos, widget->strings[widget->value], 20, COLOR_TEXT);
        } else {
            draw_text(camera, pos, widget->string, 20, COLOR_TEXT);
        }
    }
}


bool input_widgets(int camera, SDL_Event event) {
    static Vector2 mouse_position = { 0.0f, 0.0f };
    static bool mouse_down = false;
    static int grabbed_window = -1;
    static Vector2 grab_offset = { 0.0f, 0.0f };
    
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        Vector2 mouse_screen = { event.motion.x, event.motion.y };
        mouse_position = screen_to_world(camera, mouse_screen);
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        mouse_down = true;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
        mouse_down = false;
        grabbed_window = -1;
    }

    bool input_detected = false;
    int top_textbox = -1;
    for (int i = 0; i < game_data->components->entities; i++) {
        WidgetComponent* widget = WidgetComponent_get(i);
        if (!widget) continue;
        if (widget->type == WIDGET_TEXTBOX) {
            top_textbox = i;
        }
        if (!widget->selected) continue;

        CoordinateComponent* coord = CoordinateComponent_get(i);

        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if (widget->type == WIDGET_WINDOW) {
                if (i == grabbed_window) {
                    coord->position = sum(mouse_position, grab_offset);
                    return true;
                }
            } else if (widget->type == WIDGET_SLIDER) {
                if (mouse_down) {
                    set_slider(i, mouse_position);
                    return true;
                }
            } else if (widget->type == WIDGET_SCROLLBAR) {
                if (mouse_down) {
                    set_scrollbar(i, mouse_position);
                    return true;
                }
            }
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (widget->type == WIDGET_WINDOW) {
                grabbed_window = i;
                grab_offset = diff(coord->position, mouse_position);
            } else if (widget->type == WIDGET_SLIDER) {
                set_slider(i, mouse_position);
            }
            int root = get_root(i);
            bring_to_top(root);
            return true;
        } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
            if (widget->on_click) {
                widget->on_click(i);
                return true;
            }
        } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            int delta = -event.wheel.y;
            if (widget->type == WIDGET_CONTAINER || widget->type == WIDGET_SPINBOX 
                    || widget->type == WIDGET_SCROLLBAR) {
                increment_value(i, delta);
                return true;
            } else {
                if (coord->parent) {
                    WidgetComponent* parent = WidgetComponent_get(coord->parent);
                    if (parent && parent->type == WIDGET_CONTAINER) {
                        increment_value(coord->parent, delta);
                        return true;
                    }
                }
            }
        }
    }

    if (top_textbox != -1) {
        WidgetComponent* widget = WidgetComponent_get(top_textbox);
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            char* character = (char*) &event.text.text;
            int len = strlen(widget->string);
            if (len < widget->max_value - 1) {
                strcat(widget->string, character);
            }

            input_detected = true;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_BACKSPACE) {
                int len = strlen(widget->string);
                if (len > 0) {
                    widget->string[len - 1] = '\0';
                }
            }

            input_detected = true;
        }

        if (widget->on_change && input_detected) {
            widget->on_change(top_textbox, 0);
        }
    }

    return input_detected;
}


void destroy_widgets() {
    for (int i = 0; i < game_data->components->entities; i++) {
        if (WidgetComponent_get(i)) {
            destroy_entity(i);
        }
    }
}
