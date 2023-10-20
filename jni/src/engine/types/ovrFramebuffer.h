#pragma once
#include "ovrSwapChain.h"

namespace nar
{
    struct ovrFramebuffer {
        int Width;
        int Height;
        int Multisamples;
        uint32_t TextureSwapChainLength;
        uint32_t TextureSwapChainIndex;
        ovrSwapChain ColorSwapChain;
        XrSwapchainImageOpenGLESKHR *ColorSwapChainImage;
        GLuint *DepthBuffers;
        GLuint *FrameBuffers;
    };
}