#pragma once

namespace nar
{
    struct ovrEgl {
        EGLint MajorVersion;
        EGLint MinorVersion;
        EGLDisplay Display;
        EGLConfig Config;
        EGLSurface TinySurface;
        EGLSurface MainSurface;
        EGLContext Context;
    };
}