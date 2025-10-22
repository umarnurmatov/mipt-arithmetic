#pragma once

#include <stddef.h>
#include <stdint.h>

enum gui_err_t
{
    GUI_ERR_INIT_ERR,
    GUI_ERR_MEM_ALLOC_ERR,
    GUI_ERR_SUCCESS
};

enum gui_status_t
{
    GUI_STATUS_CONTINUE,
    GUI_STATUS_QUIT
};

/// @brief RGBA color
struct rgba_t
{
    uint8_t r, g, b, a;
};

static const struct rgba_t GUI_COLOR_BLACK = {0  , 0  , 0  , 255};
static const struct rgba_t GUI_COLOR_WHITE = {255, 255, 255, 255};
static const struct rgba_t GUI_COLOR_GREEN = {0  , 255, 0  , 255};

/// @brief initialize GUI backend and create window
/// @param window_width window width
/// @param window_height window height
/// @return see gui_err_t
enum gui_err_t utils_gui_init(int window_width, int window_height);

/// @brief set coordinate axes origin
/// @param origin_x x-coordinate of origin
/// @param origin_y y-coordinate of origin
void utils_gui_set_coord_origin(int origin_x, int origin_y);

/// @brief clear previously rendered objects
/// @param color color to fill
void utils_gui_clear_render(struct rgba_t color);

gui_err_t utils_gui_render_array(int* arr, int size_x, int size_y);

/// @brief show previously rendered objets in window
void utils_gui_show();

/// @brief GUI event processing loop
/// @return see gui_status_t
enum gui_status_t utils_gui_event_loop();

/// @brief destroy GUI backend
void utils_gui_end();
