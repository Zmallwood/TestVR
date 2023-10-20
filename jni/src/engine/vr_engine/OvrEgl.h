#pragma once
#include "OpenGLESUtilitiyFunctions.h"
#include "engine/Macros.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "engine/types/ovrEgl.h"

namespace nar
{
    static void ovrEgl_Clear(ovrEgl *egl) {
        egl->MajorVersion = 0;
        egl->MinorVersion = 0;
        egl->Display = 0;
        egl->Config = 0;
        egl->TinySurface = EGL_NO_SURFACE;
        egl->MainSurface = EGL_NO_SURFACE;
        egl->Context = EGL_NO_CONTEXT;
    }

    static void ovrEgl_CreateContext(ovrEgl *egl, const ovrEgl *shareEgl) {
        if (egl->Display != 0) {
            return;
        }

        egl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        ALOGV("        eglInitialize( Display, &MajorVersion, &MinorVersion )");
        eglInitialize(egl->Display, &egl->MajorVersion, &egl->MinorVersion);
        // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
        // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
        // settings, and that is completely wasted for our warp target.
        const int MAX_CONFIGS = 1024;
        EGLConfig configs[MAX_CONFIGS];
        EGLint numConfigs = 0;
        if (eglGetConfigs(egl->Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE) {
            ALOGE("        eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
            return;
        }
        const EGLint configAttribs[] = {
            EGL_RED_SIZE,
            8,
            EGL_GREEN_SIZE,
            8,
            EGL_BLUE_SIZE,
            8,
            EGL_ALPHA_SIZE,
            8, // need alpha for the multi-pass timewarp compositor
            EGL_DEPTH_SIZE,
            0,
            EGL_STENCIL_SIZE,
            0,
            EGL_SAMPLES,
            0,
            EGL_NONE};
        egl->Config = 0;
        for (int i = 0; i < numConfigs; i++) {
            EGLint value = 0;

            eglGetConfigAttrib(egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value);
            if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR) {
                continue;
            }

            // The pbuffer config also needs to be compatible with normal window rendering
            // so it can share textures with the window context.
            eglGetConfigAttrib(egl->Display, configs[i], EGL_SURFACE_TYPE, &value);
            if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) {
                continue;
            }

            int j = 0;
            for (; configAttribs[j] != EGL_NONE; j += 2) {
                eglGetConfigAttrib(egl->Display, configs[i], configAttribs[j], &value);
                if (value != configAttribs[j + 1]) {
                    break;
                }
            }
            if (configAttribs[j] == EGL_NONE) {
                egl->Config = configs[i];
                break;
            }
        }
        if (egl->Config == 0) {
            ALOGE("        eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
            return;
        }
        EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        ALOGV("        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )");
        egl->Context = eglCreateContext(
            egl->Display, egl->Config, (shareEgl != NULL) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs);
        if (egl->Context == EGL_NO_CONTEXT) {
            ALOGE("        eglCreateContext() failed: %s", EglErrorString(eglGetError()));
            return;
        }
        const EGLint surfaceAttribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};
        ALOGV("        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )");
        egl->TinySurface = eglCreatePbufferSurface(egl->Display, egl->Config, surfaceAttribs);
        if (egl->TinySurface == EGL_NO_SURFACE) {
            ALOGE("        eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
            eglDestroyContext(egl->Display, egl->Context);
            egl->Context = EGL_NO_CONTEXT;
            return;
        }
        ALOGV("        eglMakeCurrent( Display, TinySurface, TinySurface, Context )");
        if (eglMakeCurrent(egl->Display, egl->TinySurface, egl->TinySurface, egl->Context) == EGL_FALSE) {
            ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
            eglDestroySurface(egl->Display, egl->TinySurface);
            eglDestroyContext(egl->Display, egl->Context);
            egl->Context = EGL_NO_CONTEXT;
            return;
        }
    }

    static void ovrEgl_DestroyContext(ovrEgl *egl) {
        if (egl->Display != 0) {
            ALOGE("        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )");
            if (eglMakeCurrent(egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
                ALOGE("        eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
            }
        }
        if (egl->Context != EGL_NO_CONTEXT) {
            ALOGE("        eglDestroyContext( Display, Context )");
            if (eglDestroyContext(egl->Display, egl->Context) == EGL_FALSE) {
                ALOGE("        eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
            }
            egl->Context = EGL_NO_CONTEXT;
        }
        if (egl->TinySurface != EGL_NO_SURFACE) {
            ALOGE("        eglDestroySurface( Display, TinySurface )");
            if (eglDestroySurface(egl->Display, egl->TinySurface) == EGL_FALSE) {
                ALOGE("        eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
            }
            egl->TinySurface = EGL_NO_SURFACE;
        }
        if (egl->Display != 0) {
            ALOGE("        eglTerminate( Display )");
            if (eglTerminate(egl->Display) == EGL_FALSE) {
                ALOGE("        eglTerminate() failed: %s", EglErrorString(eglGetError()));
            }
            egl->Display = 0;
        }
    }
}