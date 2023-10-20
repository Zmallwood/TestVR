#pragma once
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr_platform.h>

namespace nar
{
    struct LocVel {
        XrSpaceLocation loc;
        XrSpaceVelocity vel;
    };
}