#pragma once
#include <android/log.h>

#define OVR_LOG_TAG "XrCompositor_NativeActivity"

#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, OVR_LOG_TAG, __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, OVR_LOG_TAG, __VA_ARGS__)

namespace nar
{
#ifdef CHECK_GL_ERRORS

    static const char *GlErrorString(GLenum error) {
        switch (error) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "unknown";
        }
    }

    static void GLCheckErrors(int line) {
        for (int i = 0; i < 10; i++) {
            const GLenum error = glGetError();
            if (error == GL_NO_ERROR) {
                break;
            }
            ALOGE("GL error on line %d: %s", line, GlErrorString(error));
        }
    }

#define GL(func)                                                                                                       \
    func;                                                                                                              \
    GLCheckErrors(__LINE__);

#else // CHECK_GL_ERRORS

#define GL(func) func;

#endif // CHECK_GL_ERRORS
}