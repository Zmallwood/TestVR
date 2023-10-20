#pragma once
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include "OpenXRUtilityFunctions.h"
#include "OvrApp_GetInstance.h"
#include "engine/types/ovrFramebuffer.h"
#include "engine/types/ovrSwapChain.h"
#include <openxr/openxr.h>
#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>
#include <openxr/openxr_platform.h>
#include <string.h>

namespace nar
{
#if !defined(GL_EXT_multisampled_render_to_texture)
    typedef void(GL_APIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(
        GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    typedef void(GL_APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(
        GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
#endif

    static void ovrFramebuffer_Clear(ovrFramebuffer *frameBuffer) {
        frameBuffer->Width = 0;
        frameBuffer->Height = 0;
        frameBuffer->Multisamples = 0;
        frameBuffer->TextureSwapChainLength = 0;
        frameBuffer->TextureSwapChainIndex = 0;
        frameBuffer->ColorSwapChain.Handle = XR_NULL_HANDLE;
        frameBuffer->ColorSwapChain.Width = 0;
        frameBuffer->ColorSwapChain.Height = 0;
        frameBuffer->ColorSwapChainImage = NULL;
        frameBuffer->DepthBuffers = NULL;
        frameBuffer->FrameBuffers = NULL;
    }

    static const char *GlFrameBufferStatusString(GLenum status) {
        switch (status) {
        case GL_FRAMEBUFFER_UNDEFINED:
            return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:
            return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:
            return "unknown";
        }
    }

    static bool ovrFramebuffer_Create(
        XrSession session, ovrFramebuffer *frameBuffer, const GLenum colorFormat, const int width, const int height,
        const int multisamples) {
        PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT =
            (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
        PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT =
            (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");

        frameBuffer->Width = width;
        frameBuffer->Height = height;
        frameBuffer->Multisamples = multisamples;

        GLenum requestedGLFormat = colorFormat;

        // Get the number of supported formats.
        uint32_t numInputFormats = 0;
        uint32_t numOutputFormats = 0;
        OXR(xrEnumerateSwapchainFormats(session, numInputFormats, &numOutputFormats, NULL));

        // Allocate an array large enough to contain the supported formats.
        numInputFormats = numOutputFormats;
        int64_t *supportedFormats = (int64_t *)malloc(numOutputFormats * sizeof(int64_t));
        if (supportedFormats != NULL) {
            OXR(xrEnumerateSwapchainFormats(session, numInputFormats, &numOutputFormats, supportedFormats));
        }

        // Verify the requested format is supported.
        uint64_t selectedFormat = 0;
        for (uint32_t i = 0; i < numOutputFormats; i++) {
            if (supportedFormats[i] == requestedGLFormat) {
                selectedFormat = supportedFormats[i];
                break;
            }
        }

        free(supportedFormats);

        if (selectedFormat == 0) {
            ALOGE("Format not supported");
        }

        XrSwapchainCreateInfo swapChainCreateInfo;
        memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
        swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        swapChainCreateInfo.format = selectedFormat;
        swapChainCreateInfo.sampleCount = 1;
        swapChainCreateInfo.width = width;
        swapChainCreateInfo.height = height;
        swapChainCreateInfo.faceCount = 1;
        swapChainCreateInfo.arraySize = 1;
        swapChainCreateInfo.mipCount = 1;

        // Enable Foveation on this swapchain
        XrSwapchainCreateInfoFoveationFB swapChainFoveationCreateInfo;
        memset(&swapChainFoveationCreateInfo, 0, sizeof(swapChainFoveationCreateInfo));
        swapChainFoveationCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO_FOVEATION_FB;
        swapChainCreateInfo.next = &swapChainFoveationCreateInfo;

        frameBuffer->ColorSwapChain.Width = swapChainCreateInfo.width;
        frameBuffer->ColorSwapChain.Height = swapChainCreateInfo.height;

        // Create the swapchain.
        OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &frameBuffer->ColorSwapChain.Handle));
        // Get the number of swapchain images.
        OXR(xrEnumerateSwapchainImages(
            frameBuffer->ColorSwapChain.Handle, 0, &frameBuffer->TextureSwapChainLength, NULL));
        // Allocate the swapchain images array.
        frameBuffer->ColorSwapChainImage = (XrSwapchainImageOpenGLESKHR *)malloc(
            frameBuffer->TextureSwapChainLength * sizeof(XrSwapchainImageOpenGLESKHR));

        // Populate the swapchain image array.
        for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
            frameBuffer->ColorSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
            frameBuffer->ColorSwapChainImage[i].next = NULL;
        }
        OXR(xrEnumerateSwapchainImages(
            frameBuffer->ColorSwapChain.Handle, frameBuffer->TextureSwapChainLength,
            &frameBuffer->TextureSwapChainLength, (XrSwapchainImageBaseHeader *)frameBuffer->ColorSwapChainImage));

        frameBuffer->DepthBuffers = (GLuint *)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));
        frameBuffer->FrameBuffers = (GLuint *)malloc(frameBuffer->TextureSwapChainLength * sizeof(GLuint));

        for (uint32_t i = 0; i < frameBuffer->TextureSwapChainLength; i++) {
            // Create the color buffer texture.
            const GLuint colorTexture = frameBuffer->ColorSwapChainImage[i].image;

            GLenum colorTextureTarget = GL_TEXTURE_2D;
            GL(glBindTexture(colorTextureTarget, colorTexture));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL(glTexParameteri(colorTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL(glBindTexture(colorTextureTarget, 0));

            if (multisamples > 1 && glRenderbufferStorageMultisampleEXT != NULL &&
                glFramebufferTexture2DMultisampleEXT != NULL) {
                // Create multisampled depth buffer.
                GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glRenderbufferStorageMultisampleEXT(
                    GL_RENDERBUFFER, multisamples, GL_DEPTH_COMPONENT24, width, height));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

                // Create the frame buffer.
                // NOTE: glFramebufferTexture2DMultisampleEXT only works with GL_FRAMEBUFFER.
                GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
                GL(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
                GL(glFramebufferTexture2DMultisampleEXT(
                    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0, multisamples));
                GL(glFramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER));
                GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                    ALOGE("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            }
            else {
                // Create depth buffer.
                GL(glGenRenderbuffers(1, &frameBuffer->DepthBuffers[i]));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height));
                GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));

                // Create the frame buffer.
                GL(glGenFramebuffers(1, &frameBuffer->FrameBuffers[i]));
                GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[i]));
                GL(glFramebufferRenderbuffer(
                    GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frameBuffer->DepthBuffers[i]));
                GL(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0));
                GL(GLenum renderFramebufferStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));
                GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
                if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
                    ALOGE("Incomplete frame buffer object: %s", GlFrameBufferStatusString(renderFramebufferStatus));
                    return false;
                }
            }
        }

        return true;
    }

    static void ovrFramebuffer_Destroy(ovrFramebuffer *frameBuffer) {
        GL(glDeleteFramebuffers(frameBuffer->TextureSwapChainLength, frameBuffer->FrameBuffers));
        GL(glDeleteRenderbuffers(frameBuffer->TextureSwapChainLength, frameBuffer->DepthBuffers));
        OXR(xrDestroySwapchain(frameBuffer->ColorSwapChain.Handle));
        free(frameBuffer->ColorSwapChainImage);

        free(frameBuffer->DepthBuffers);
        free(frameBuffer->FrameBuffers);

        ovrFramebuffer_Clear(frameBuffer);
    }

    static void ovrFramebuffer_SetCurrent(ovrFramebuffer *frameBuffer) {
        GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBuffer->FrameBuffers[frameBuffer->TextureSwapChainIndex]));
    }

    static void ovrFramebuffer_SetNone() {
        GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
    }

    static void ovrFramebuffer_Resolve(ovrFramebuffer *frameBuffer) {
        // Discard the depth buffer, so the tiler won't need to write it back out to memory.
        const GLenum depthAttachment[1] = {GL_DEPTH_ATTACHMENT};
        glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, depthAttachment);

        // We now let the resolve happen implicitly.
    }

    static void ovrFramebuffer_Acquire(ovrFramebuffer *frameBuffer) {
        // Acquire the swapchain image
        XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
        OXR(xrAcquireSwapchainImage(
            frameBuffer->ColorSwapChain.Handle, &acquireInfo, &frameBuffer->TextureSwapChainIndex));

        XrSwapchainImageWaitInfo waitInfo;
        waitInfo.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        waitInfo.next = NULL;
        waitInfo.timeout = 1000000000; /* timeout in nanoseconds */
        XrResult res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
        int i = 0;
        while (res == XR_TIMEOUT_EXPIRED) {
            res = xrWaitSwapchainImage(frameBuffer->ColorSwapChain.Handle, &waitInfo);
            i++;
            ALOGV(
                " Retry xrWaitSwapchainImage %d times due to XR_TIMEOUT_EXPIRED (duration %f seconds)", i,
                waitInfo.timeout * (1E-9));
        }
    }

    static void ovrFramebuffer_Release(ovrFramebuffer *frameBuffer) {
        XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
        OXR(xrReleaseSwapchainImage(frameBuffer->ColorSwapChain.Handle, &releaseInfo));
    }
}