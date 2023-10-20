#pragma once
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr_platform.h>
#include "engine/Macros.h"

namespace nar
{
#define DEBUG 1
#if defined(DEBUG)
    static void OXR_CheckErrors(XrInstance instance, XrResult result, const char *function, bool failOnError) {
        if (XR_FAILED(result)) {
            char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
            xrResultToString(instance, result, errorBuffer);
            if (failOnError) {
                ALOGE("OpenXR error: %s: %s\n", function, errorBuffer);
            }
            else {
                ALOGV("OpenXR error: %s: %s\n", function, errorBuffer);
            }
        }
    }
#endif

#if defined(DEBUG)
#define OXR(func) OXR_CheckErrors(ovrApp_GetInstance(), func, #func, true);
#else
#define OXR(func) OXR_CheckErrors(ovrApp_GetInstance(), func, #func, false);
#endif
}