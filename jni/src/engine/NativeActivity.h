#pragma once
#include <cstdint>

struct android_app;

namespace nar
{
    /**
     * Process the next main command.
     */
    extern void app_handle_cmd(android_app *app, int32_t cmd);
}