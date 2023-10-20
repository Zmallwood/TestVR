#pragma once

namespace nar
{
    struct ovrSwapChain {
        XrSwapchain Handle;
        uint32_t Width;
        uint32_t Height;
    };
}