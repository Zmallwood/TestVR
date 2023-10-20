#pragma once
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr_platform.h>

namespace nar
{
    union ovrCompositorLayer_Union {
        XrCompositionLayerProjection Projection;
        XrCompositionLayerQuad Quad;
        XrCompositionLayerCylinderKHR Cylinder;
        XrCompositionLayerCubeKHR Cube;
        XrCompositionLayerEquirect2KHR Equirect2;
    };
}