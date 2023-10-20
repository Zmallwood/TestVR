#pragma once
#include "KTXLoading.h"
#include "OpenXRUtilityFunctions.h"
#include "OvrApp_GetInstance.h"
#include "gl/GeometryOperations.h"
#include "gl/GLProgramOperations.h"
#include "input/TrackedControllerOperations.h"
#include "engine/Global.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <android/native_window_jni.h>
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr_platform.h>

// EXT_texture_border_clamp
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

namespace nar
{
    static void ovrScene_Clear(ovrScene *scene) {
        scene->CreatedScene = false;
        scene->CreatedVAOs = false;
        ovrGLProgram_Clear(&scene->Program);
        ovrGeometry_Clear(&scene->GroundPlane);
        ovrGeometry_Clear(&scene->Box);
        for (int i = 0; i < 4; i++) {
            ovrTrackedController_Clear(&scene->TrackedController[i]);
        }

        scene->CubeMapSwapChain.Handle = XR_NULL_HANDLE;
        scene->CubeMapSwapChain.Width = 0;
        scene->CubeMapSwapChain.Height = 0;
        scene->CubeMapSwapChainImage = NULL;
        scene->EquirectSwapChain.Handle = XR_NULL_HANDLE;
        scene->EquirectSwapChain.Width = 0;
        scene->EquirectSwapChain.Height = 0;
        scene->EquirectSwapChainImage = NULL;
        scene->CylinderSwapChain.Handle = XR_NULL_HANDLE;
        scene->CylinderSwapChain.Width = 0;
        scene->CylinderSwapChain.Height = 0;
        scene->CylinderSwapChainImage = NULL;
        scene->QuadSwapChain.Handle = XR_NULL_HANDLE;
        scene->QuadSwapChain.Width = 0;
        scene->QuadSwapChain.Height = 0;
        scene->QuadSwapChainImage = NULL;

        scene->BackGroundType = BACKGROUND_EQUIRECT;
    }

    static bool ovrScene_IsCreated(ovrScene *scene) {
        return scene->CreatedScene;
    }

    static void ovrScene_CreateVAOs(ovrScene *scene) {
        if (!scene->CreatedVAOs) {
            ovrGeometry_CreateVAO(&scene->GroundPlane);
            ovrGeometry_CreateVAO(&scene->Box);
            scene->CreatedVAOs = true;
        }
    }

    static void ovrScene_DestroyVAOs(ovrScene *scene) {
        if (scene->CreatedVAOs) {
            ovrGeometry_DestroyVAO(&scene->GroundPlane);
            ovrGeometry_DestroyVAO(&scene->Box);
            scene->CreatedVAOs = false;
        }
    }

    static void ovrScene_Create(AAssetManager *amgr, XrInstance instance, XrSession session, ovrScene *scene) {
        // Simple ground plane and box geometry.
        {
            ovrGLProgram_Create(&scene->Program, VERTEX_SHADER, FRAGMENT_SHADER);
            ovrGeometry_CreateGroundPlane(&scene->GroundPlane);
            ovrGeometry_CreateBox(&scene->Box);

            ovrScene_CreateVAOs(scene);
        }

        // Simple cubemap loaded from ktx file on the sdcard. NOTE: Currently only
        // handles texture2d or cubemap types.
        {
            ktxImageInfo_t ktxImageInfo;
            memset(&ktxImageInfo, 0, sizeof(ktxImageInfo_t));
            if (LoadImageDataFromKTXFile(amgr, "cubemap256.ktx", &ktxImageInfo) == true) {
                int swapChainTexId = 0;
                XrSwapchainCreateInfo swapChainCreateInfo;
                memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
                swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
                swapChainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
                swapChainCreateInfo.usageFlags =
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                swapChainCreateInfo.format = ktxImageInfo.internalFormat;
                swapChainCreateInfo.sampleCount = 1;
                swapChainCreateInfo.width = ktxImageInfo.width;
                swapChainCreateInfo.height = ktxImageInfo.height;
                swapChainCreateInfo.faceCount = ktxImageInfo.numberOfFaces;
                swapChainCreateInfo.arraySize = ktxImageInfo.numberOfArrayElements;
                swapChainCreateInfo.mipCount = ktxImageInfo.numberOfMipmapLevels;

                scene->CubeMapSwapChain.Width = swapChainCreateInfo.width;
                scene->CubeMapSwapChain.Height = swapChainCreateInfo.height;

                // Create the swapchain.
                OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &scene->CubeMapSwapChain.Handle));
                // Get the number of swapchain images.
                uint32_t length;
                OXR(xrEnumerateSwapchainImages(scene->CubeMapSwapChain.Handle, 0, &length, NULL));
                scene->CubeMapSwapChainImage =
                    (XrSwapchainImageOpenGLESKHR *)malloc(length * sizeof(XrSwapchainImageOpenGLESKHR));
                for (uint32_t i = 0; i < length; i++) {
                    scene->CubeMapSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
                    scene->CubeMapSwapChainImage[i].next = NULL;
                }
                OXR(xrEnumerateSwapchainImages(
                    scene->CubeMapSwapChain.Handle, length, &length,
                    (XrSwapchainImageBaseHeader *)scene->CubeMapSwapChainImage));

                swapChainTexId = scene->CubeMapSwapChainImage[0].image;

                if (LoadTextureFromKTXImageMemory(&ktxImageInfo, swapChainTexId) == false) {
                    ALOGE("Failed to load texture from KTX image");
                }

                uint32_t index = 0;
                XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
                OXR(xrAcquireSwapchainImage(scene->CubeMapSwapChain.Handle, &acquireInfo, &index));

                XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, NULL, 0};
                OXR(xrWaitSwapchainImage(scene->CubeMapSwapChain.Handle, &waitInfo));

                XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
                OXR(xrReleaseSwapchainImage(scene->CubeMapSwapChain.Handle, &releaseInfo));
            }
            else {
                ALOGE("Failed to load KTX image - generating procedural cubemap");
                XrSwapchainCreateInfo swapChainCreateInfo;
                memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
                swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
                swapChainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
                swapChainCreateInfo.usageFlags =
                    XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
                swapChainCreateInfo.format = GL_RGBA8;
                swapChainCreateInfo.sampleCount = 1;
                int w = 512;
                swapChainCreateInfo.width = w;
                swapChainCreateInfo.height = w;
                swapChainCreateInfo.faceCount = 6;
                swapChainCreateInfo.arraySize = 1;
                swapChainCreateInfo.mipCount = 1;

                scene->CubeMapSwapChain.Width = w;
                scene->CubeMapSwapChain.Height = w;

                // Create the swapchain.
                OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &scene->CubeMapSwapChain.Handle));
                // Get the number of swapchain images.
                uint32_t length;
                OXR(xrEnumerateSwapchainImages(scene->CubeMapSwapChain.Handle, 0, &length, NULL));
                scene->CubeMapSwapChainImage =
                    (XrSwapchainImageOpenGLESKHR *)malloc(length * sizeof(XrSwapchainImageOpenGLESKHR));
                for (uint32_t i = 0; i < length; i++) {
                    scene->CubeMapSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
                    scene->CubeMapSwapChainImage[i].next = NULL;
                }
                OXR(xrEnumerateSwapchainImages(
                    scene->CubeMapSwapChain.Handle, length, &length,
                    (XrSwapchainImageBaseHeader *)scene->CubeMapSwapChainImage));

                uint32_t *img = (uint32_t *)malloc(w * w * sizeof(uint32_t));
                for (int j = 0; j < w; j++) {
                    for (int i = 0; i < w; i++) {
                        img[j * w + i] = 0xff000000 + (((j * 256) / w) << 8) + (((i * 256) / w) << 0);
                    }
                }
                GL(glBindTexture(GL_TEXTURE_CUBE_MAP, scene->CubeMapSwapChainImage[0].image));
                const int start = 3 * w / 8;
                const int stop = 5 * w / 8;
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xff0000ff;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xffffff00;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xff00ff00;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xffff00ff;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xffff0000;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));
                for (int j = start; j < stop; j++) {
                    for (int i = start; i < stop; i++) {
                        img[j * w + i] = 0xff00ffff;
                    }
                }
                GL(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 0, 0, w, w, GL_RGBA, GL_UNSIGNED_BYTE, img));

                free(img);

                uint32_t index = 0;
                XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
                OXR(xrAcquireSwapchainImage(scene->CubeMapSwapChain.Handle, &acquireInfo, &index));

                XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, NULL, 0};
                OXR(xrWaitSwapchainImage(scene->CubeMapSwapChain.Handle, &waitInfo));

                XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
                OXR(xrReleaseSwapchainImage(scene->CubeMapSwapChain.Handle, &releaseInfo));
            }
        }

        // Simple checkerboard pattern.
        {
            const int width = 512;
            const int height = 256;
            XrSwapchainCreateInfo swapChainCreateInfo;
            memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
            swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
            swapChainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
            swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapChainCreateInfo.format = GL_SRGB8_ALPHA8;
            swapChainCreateInfo.sampleCount = 1;
            swapChainCreateInfo.width = width;
            swapChainCreateInfo.height = height;
            swapChainCreateInfo.faceCount = 1;
            swapChainCreateInfo.arraySize = 1;
            swapChainCreateInfo.mipCount = 1;

            scene->EquirectSwapChain.Width = swapChainCreateInfo.width;
            scene->EquirectSwapChain.Height = swapChainCreateInfo.height;

            // Create the swapchain.
            OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &scene->EquirectSwapChain.Handle));
            // Get the number of swapchain images.
            uint32_t length;
            OXR(xrEnumerateSwapchainImages(scene->EquirectSwapChain.Handle, 0, &length, NULL));
            scene->EquirectSwapChainImage =
                (XrSwapchainImageOpenGLESKHR *)malloc(length * sizeof(XrSwapchainImageOpenGLESKHR));

            for (uint32_t i = 0; i < length; i++) {
                scene->EquirectSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
                scene->EquirectSwapChainImage[i].next = NULL;
            }
            OXR(xrEnumerateSwapchainImages(
                scene->EquirectSwapChain.Handle, length, &length,
                (XrSwapchainImageBaseHeader *)scene->EquirectSwapChainImage));

            uint32_t *texData = (uint32_t *)malloc(width * height * sizeof(uint32_t));

            for (int y = 0; y < height; y++) {
                float greenfrac = y / (height - 1.0f);
                uint32_t green = ((uint32_t)(255 * greenfrac)) << 8;
                for (int x = 0; x < width; x++) {
                    float redfrac = x / (width - 1.0f);
                    uint32_t red = ((uint32_t)(255 * redfrac));
                    texData[y * width + x] = (x ^ y) & 16 ? 0xFFFFFFFF : (0xFF000000 | green | red);
                }
            }

            const int texId = scene->EquirectSwapChainImage[0].image;
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, texData);
            glBindTexture(GL_TEXTURE_2D, 0);
            free(texData);

            uint32_t index = 0;
            XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
            OXR(xrAcquireSwapchainImage(scene->EquirectSwapChain.Handle, &acquireInfo, &index));

            XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, NULL, 0};
            OXR(xrWaitSwapchainImage(scene->EquirectSwapChain.Handle, &waitInfo));

            XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
            OXR(xrReleaseSwapchainImage(scene->EquirectSwapChain.Handle, &releaseInfo));
        }

        // Simple checkerboard pattern.
        {
            static const int CYLINDER_WIDTH = 512;
            static const int CYLINDER_HEIGHT = 128;

            XrSwapchainCreateInfo swapChainCreateInfo;
            memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
            swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
            swapChainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
            swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapChainCreateInfo.format = GL_SRGB8_ALPHA8;
            swapChainCreateInfo.sampleCount = 1;
            swapChainCreateInfo.width = CYLINDER_WIDTH;
            swapChainCreateInfo.height = CYLINDER_HEIGHT;
            swapChainCreateInfo.faceCount = 1;
            swapChainCreateInfo.arraySize = 1;
            swapChainCreateInfo.mipCount = 1;

            scene->CylinderSwapChain.Width = swapChainCreateInfo.width;
            scene->CylinderSwapChain.Height = swapChainCreateInfo.height;

            // Create the swapchain.
            OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &scene->CylinderSwapChain.Handle));
            // Get the number of swapchain images.
            uint32_t length;
            OXR(xrEnumerateSwapchainImages(scene->CylinderSwapChain.Handle, 0, &length, NULL));
            scene->CylinderSwapChainImage =
                (XrSwapchainImageOpenGLESKHR *)malloc(length * sizeof(XrSwapchainImageOpenGLESKHR));
            for (uint32_t i = 0; i < length; i++) {
                scene->CylinderSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
                scene->CylinderSwapChainImage[i].next = NULL;
            }
            OXR(xrEnumerateSwapchainImages(
                scene->CylinderSwapChain.Handle, length, &length,
                (XrSwapchainImageBaseHeader *)scene->CylinderSwapChainImage));

            uint32_t *texData = (uint32_t *)malloc(CYLINDER_WIDTH * CYLINDER_HEIGHT * sizeof(uint32_t));

            for (int y = 0; y < CYLINDER_HEIGHT; y++) {
                for (int x = 0; x < CYLINDER_WIDTH; x++) {
                    texData[y * CYLINDER_WIDTH + x] = (x ^ y) & 64 ? 0xFF6464F0 : 0xFFF06464;
                }
            }
            for (int y = 0; y < CYLINDER_HEIGHT; y++) {
                int g = 255.0f * (y / (CYLINDER_HEIGHT - 1.0f));
                texData[y * CYLINDER_WIDTH] = 0xff000000 | (g << 8);
            }
            for (int x = 0; x < CYLINDER_WIDTH; x++) {
                int r = 255.0f * (x / (CYLINDER_WIDTH - 1.0f));
                texData[x] = 0xff000000 | r;
            }

            const int texId = scene->CylinderSwapChainImage[0].image;

            glBindTexture(GL_TEXTURE_2D, texId);
            glTexSubImage2D(
                GL_TEXTURE_2D, 0, 0, 0, CYLINDER_WIDTH, CYLINDER_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, texData);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindTexture(GL_TEXTURE_2D, 0);

            free(texData);

            uint32_t index = 0;
            XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
            OXR(xrAcquireSwapchainImage(scene->CylinderSwapChain.Handle, &acquireInfo, &index));

            XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, NULL, 0};
            OXR(xrWaitSwapchainImage(scene->CylinderSwapChain.Handle, &waitInfo));

            XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
            OXR(xrReleaseSwapchainImage(scene->CylinderSwapChain.Handle, &releaseInfo));
        }

        // Simple checkerboard pattern.
        {
            static const int QUAD_WIDTH = 256;
            static const int QUAD_HEIGHT = 256;

            XrSwapchainCreateInfo swapChainCreateInfo;
            memset(&swapChainCreateInfo, 0, sizeof(swapChainCreateInfo));
            swapChainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
            swapChainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
            swapChainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapChainCreateInfo.format = GL_SRGB8_ALPHA8;
            swapChainCreateInfo.sampleCount = 1;
            swapChainCreateInfo.width = QUAD_WIDTH;
            swapChainCreateInfo.height = QUAD_HEIGHT;
            swapChainCreateInfo.faceCount = 1;
            swapChainCreateInfo.arraySize = 1;
            swapChainCreateInfo.mipCount = 1;

            scene->QuadSwapChain.Width = swapChainCreateInfo.width;
            scene->QuadSwapChain.Height = swapChainCreateInfo.height;

            // Create the swapchain.
            OXR(xrCreateSwapchain(session, &swapChainCreateInfo, &scene->QuadSwapChain.Handle));
            // Get the number of swapchain images.
            uint32_t length;
            OXR(xrEnumerateSwapchainImages(scene->QuadSwapChain.Handle, 0, &length, NULL));
            scene->QuadSwapChainImage =
                (XrSwapchainImageOpenGLESKHR *)malloc(length * sizeof(XrSwapchainImageOpenGLESKHR));
            for (uint32_t i = 0; i < length; i++) {
                scene->QuadSwapChainImage[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
                scene->QuadSwapChainImage[i].next = NULL;
            }
            OXR(xrEnumerateSwapchainImages(
                scene->QuadSwapChain.Handle, length, &length, (XrSwapchainImageBaseHeader *)scene->QuadSwapChainImage));

            uint32_t *texData = (uint32_t *)malloc(QUAD_WIDTH * QUAD_HEIGHT * sizeof(uint32_t));

            for (int y = 0; y < QUAD_HEIGHT; y++) {
                for (int x = 0; x < QUAD_WIDTH; x++) {
                    uint32_t gray = ((x ^ (x >> 1)) ^ (y ^ (y >> 1))) & 0xff;
                    uint32_t r = gray + 255.0f * ((1.0f - (gray / 255.0f)) * (x / (QUAD_WIDTH - 1.0f)));
                    uint32_t g = gray + 255.0f * ((1.0f - (gray / 255.0f)) * (y / (QUAD_HEIGHT - 1.0f)));
                    uint32_t rgba = (0xffu << 24) | (gray << 16) | (g << 8) | r;
                    texData[y * QUAD_WIDTH + x] = rgba;
                }
            }

            const int texId = scene->QuadSwapChainImage[0].image;

            glBindTexture(GL_TEXTURE_2D, texId);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, QUAD_WIDTH, QUAD_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, texData);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            GLfloat borderColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindTexture(GL_TEXTURE_2D, 0);

            free(texData);

            uint32_t index = 0;
            XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, NULL};
            OXR(xrAcquireSwapchainImage(scene->QuadSwapChain.Handle, &acquireInfo, &index));

            XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, NULL, 0};
            OXR(xrWaitSwapchainImage(scene->QuadSwapChain.Handle, &waitInfo));

            XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, NULL};
            OXR(xrReleaseSwapchainImage(scene->QuadSwapChain.Handle, &releaseInfo));
        }

        scene->CreatedScene = true;
    }

    static void ovrScene_Destroy(ovrScene *scene) {
        ovrScene_DestroyVAOs(scene);

        ovrGLProgram_Destroy(&scene->Program);
        ovrGeometry_Destroy(&scene->GroundPlane);
        ovrGeometry_Destroy(&scene->Box);

        // Cubemap is optional
        if (scene->CubeMapSwapChain.Handle != XR_NULL_HANDLE) {
            OXR(xrDestroySwapchain(scene->CubeMapSwapChain.Handle));
        }
        if (scene->CubeMapSwapChainImage != NULL) {
            free(scene->CubeMapSwapChainImage);
        }
        OXR(xrDestroySwapchain(scene->EquirectSwapChain.Handle));
        free(scene->EquirectSwapChainImage);
        OXR(xrDestroySwapchain(scene->CylinderSwapChain.Handle));
        free(scene->CylinderSwapChainImage);
        OXR(xrDestroySwapchain(scene->QuadSwapChain.Handle));
        free(scene->QuadSwapChainImage);

        scene->CreatedScene = false;
    }
}