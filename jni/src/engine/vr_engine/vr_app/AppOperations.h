#pragma once
#include <EGL/egl.h>
#include <android/native_window_jni.h>
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr.h>

namespace nar
{
    struct ovrApp;

    extern void ovrApp_Clear(ovrApp *app);

    extern void ovrApp_Destroy(ovrApp *app);

    extern void ovrApp_HandleSessionStateChanges(ovrApp *app, XrSessionState state);

    extern void ovrApp_HandleXrEvents(ovrApp *app);
}