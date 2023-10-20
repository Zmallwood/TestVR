#pragma once
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>

namespace nar
{
    struct ovrTrackedController {
        bool Active;
        XrPosef Pose;
    };
}