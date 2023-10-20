#pragma once
#include "Angelos.h"
#include "FrameBufferOperations.h"
#include "SceneOperations.h"
#include "engine/Constants.h"

namespace nar
{
    static void ovrRenderer_Clear(ovrRenderer *renderer) {
        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer_Clear(&renderer->FrameBuffer[eye]);
        }
    }

    static void ovrRenderer_Create(
        XrSession session, ovrRenderer *renderer, int suggestedEyeTextureWidth, int suggestedEyeTextureHeight) {
        // Create the frame buffers.
        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer_Create(
                session, &renderer->FrameBuffer[eye], GL_SRGB8_ALPHA8, suggestedEyeTextureWidth,
                suggestedEyeTextureHeight, NUM_MULTI_SAMPLES);
        }
    }

    static void ovrRenderer_Destroy(ovrRenderer *renderer) {
        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer_Destroy(&renderer->FrameBuffer[eye]);
        }
    }

    struct ovrSceneMatrices {
        XrMatrix4x4f ViewMatrix[ovrMaxNumEyes];
        XrMatrix4x4f ProjectionMatrix[ovrMaxNumEyes];
    };

    static void ovrRenderer_RenderFrame(
        JNIEnv *Env, jobject activity, ovrRenderer *renderer, const ovrScene *scene,
        const ovrSceneMatrices *sceneMatrices) {
        // Let the background layer show through if one is present.
        float clearAlpha = 1.0f;
        if (scene->BackGroundType != BACKGROUND_NONE) {
            clearAlpha = 0.0f;
        }

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            ovrFramebuffer *frameBuffer = &renderer->FrameBuffer[eye];

            ovrFramebuffer_Acquire(frameBuffer);

            // Set the current framebuffer.
            ovrFramebuffer_SetCurrent(frameBuffer);

            jfloatArray view1_array = Env->NewFloatArray(32);
            Env->SetFloatArrayRegion(view1_array, 0, 16, (float *)&sceneMatrices->ViewMatrix[eye]);
            Env->SetFloatArrayRegion(view1_array, 16, 16, (float *)&sceneMatrices->ProjectionMatrix[eye]);
            Env->CallVoidMethod(activity, draw_method, frameBuffer->Width, frameBuffer->Height, view1_array, eye);
            Env->DeleteLocalRef(view1_array);

            /*GL(glUseProgram(scene->Program.Program));

            XrMatrix4x4f modelMatrix;
            XrMatrix4x4f_CreateIdentity(&modelMatrix);
            glUniformMatrix4fv(
                scene->Program.UniformLocation[MODEL_MATRIX], 1, GL_FALSE, &modelMatrix.m[0]);
            XrMatrix4x4f viewProjMatrix;
            XrMatrix4x4f_Multiply(
                &viewProjMatrix,
                &sceneMatrices->ProjectionMatrix[eye],
                &sceneMatrices->ViewMatrix[eye]);
            glUniformMatrix4fv(
                scene->Program.UniformLocation[VIEW_PROJ_MATRIX], 1, GL_FALSE, &viewProjMatrix.m[0]);

            GL(glEnable(GL_SCISSOR_TEST));
            GL(glDepthMask(GL_TRUE));
            GL(glEnable(GL_DEPTH_TEST));
            GL(glDepthFunc(GL_LEQUAL));
            GL(glDisable(GL_CULL_FACE));
            // GL( glCullFace( GL_BACK ) );
            GL(glViewport(0, 0, frameBuffer->Width, frameBuffer->Height));
            GL(glScissor(0, 0, frameBuffer->Width, frameBuffer->Height));
            GL(glClearColor(0.0f, 0.0f, 0.0f, clearAlpha));
            GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
            GL(glBindVertexArray(scene->GroundPlane.VertexArrayObject));
            GL(glDrawElements(GL_TRIANGLES, scene->GroundPlane.IndexCount, GL_UNSIGNED_SHORT, NULL));

            for (int i = 0; i < 4; i++) {
                if (scene->TrackedController[i].Active == false) {
                    continue;
                }
                XrMatrix4x4f pose =
                    XrMatrix4x4f_CreateFromRigidTransform(&scene->TrackedController[i].Pose);
                XrMatrix4x4f scale;
                if (i & 1) {
                    XrMatrix4x4f_CreateScale(&scale, 0.03f, 0.03f, 0.03f);
                } else {
                    XrMatrix4x4f_CreateScale(&scale, 0.02f, 0.02f, 0.06f);
                }
                XrMatrix4x4f model;
                XrMatrix4x4f_Multiply(&model, &pose, &scale);
                glUniformMatrix4fv(
                    scene->Program.UniformLocation[MODEL_MATRIX], 1, GL_FALSE, &model.m[0]);
                GL(glBindVertexArray(scene->Box.VertexArrayObject));
                GL(glDrawElements(GL_TRIANGLES, scene->Box.IndexCount, GL_UNSIGNED_SHORT, NULL));
            }
            glUniformMatrix4fv(
                scene->Program.UniformLocation[MODEL_MATRIX], 1, GL_FALSE, &modelMatrix.m[0]);

            GL(glBindVertexArray(0));
            GL(glUseProgram(0));*/

            ovrFramebuffer_Resolve(frameBuffer);

            ovrFramebuffer_Release(frameBuffer);
        }

        ovrFramebuffer_SetNone();
    }

    static void ovrRenderer_SetFoveation(
        XrInstance *instance, XrSession *session, ovrRenderer *renderer, XrFoveationLevelFB level, float verticalOffset,
        XrFoveationDynamicFB dynamic) {
        PFN_xrCreateFoveationProfileFB pfnCreateFoveationProfileFB;
        OXR(xrGetInstanceProcAddr(
            *instance, "xrCreateFoveationProfileFB", (PFN_xrVoidFunction *)(&pfnCreateFoveationProfileFB)));

        PFN_xrDestroyFoveationProfileFB pfnDestroyFoveationProfileFB;
        OXR(xrGetInstanceProcAddr(
            *instance, "xrDestroyFoveationProfileFB", (PFN_xrVoidFunction *)(&pfnDestroyFoveationProfileFB)));

        PFN_xrUpdateSwapchainFB pfnUpdateSwapchainFB;
        OXR(xrGetInstanceProcAddr(*instance, "xrUpdateSwapchainFB", (PFN_xrVoidFunction *)(&pfnUpdateSwapchainFB)));

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            XrFoveationLevelProfileCreateInfoFB levelProfileCreateInfo;
            memset(&levelProfileCreateInfo, 0, sizeof(levelProfileCreateInfo));
            levelProfileCreateInfo.type = XR_TYPE_FOVEATION_LEVEL_PROFILE_CREATE_INFO_FB;
            levelProfileCreateInfo.level = level;
            levelProfileCreateInfo.verticalOffset = verticalOffset;
            levelProfileCreateInfo.dynamic = dynamic;

            XrFoveationProfileCreateInfoFB profileCreateInfo;
            memset(&profileCreateInfo, 0, sizeof(profileCreateInfo));
            profileCreateInfo.type = XR_TYPE_FOVEATION_PROFILE_CREATE_INFO_FB;
            profileCreateInfo.next = &levelProfileCreateInfo;

            XrFoveationProfileFB foveationProfile;

            pfnCreateFoveationProfileFB(*session, &profileCreateInfo, &foveationProfile);

            XrSwapchainStateFoveationFB foveationUpdateState;
            memset(&foveationUpdateState, 0, sizeof(foveationUpdateState));
            foveationUpdateState.type = XR_TYPE_SWAPCHAIN_STATE_FOVEATION_FB;
            foveationUpdateState.profile = foveationProfile;

            pfnUpdateSwapchainFB(
                renderer->FrameBuffer[eye].ColorSwapChain.Handle,
                (XrSwapchainStateBaseHeaderFB *)(&foveationUpdateState));

            pfnDestroyFoveationProfileFB(foveationProfile);
        }
    }
}