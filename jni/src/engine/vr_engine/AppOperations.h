#pragma once
#include "EglOperations.h"
#include "RendererOperations.h"
#include "SceneOperations.h"
#include "engine/Constants.h"
#include <assert.h>
#include "engine/types/ovrApp.h"

namespace nar
{
    static void ovrApp_Clear(ovrApp *app) {
        app->NativeWindow = NULL;
        app->Resumed = false;
        app->Focused = false;
        app->Instance = XR_NULL_HANDLE;
        app->Session = XR_NULL_HANDLE;
        memset(&app->ViewportConfig, 0, sizeof(XrViewConfigurationProperties));
        memset(&app->ViewConfigurationView, 0, ovrMaxNumEyes * sizeof(XrViewConfigurationView));
        app->SystemId = XR_NULL_SYSTEM_ID;
        app->HeadSpace = XR_NULL_HANDLE;
        app->LocalSpace = XR_NULL_HANDLE;
        app->StageSpace = XR_NULL_HANDLE;
        app->FakeStageSpace = XR_NULL_HANDLE;
        app->CurrentSpace = XR_NULL_HANDLE;
        app->SessionActive = false;
        app->SupportedDisplayRefreshRates = NULL;
        app->RequestedDisplayRefreshRateIndex = 0;
        app->NumSupportedDisplayRefreshRates = 0;
        app->pfnGetDisplayRefreshRate = NULL;
        app->pfnRequestDisplayRefreshRate = NULL;
        app->SwapInterval = 1;
        memset(app->Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);
        app->LayerCount = 0;
        app->CpuLevel = 2;
        app->GpuLevel = 2;
        app->MainThreadTid = 0;
        app->RenderThreadTid = 0;
        app->TouchPadDownLastFrame = false;

        ovrEgl_Clear(&app->Egl);
        ovrScene_Clear(&app->Scene);
        ovrRenderer_Clear(&app->Renderer);
    }

    static void ovrApp_Destroy(ovrApp *app) {
        if (app->SupportedDisplayRefreshRates != NULL) {
            free(app->SupportedDisplayRefreshRates);
        }

        ovrApp_Clear(app);
    }

    static void ovrApp_HandleSessionStateChanges(ovrApp *app, XrSessionState state) {
        if (state == XR_SESSION_STATE_READY) {
            assert(app->Resumed);
            assert(app->NativeWindow != NULL);
            assert(app->SessionActive == false);

            XrSessionBeginInfo sessionBeginInfo;
            memset(&sessionBeginInfo, 0, sizeof(sessionBeginInfo));
            sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
            sessionBeginInfo.next = NULL;
            sessionBeginInfo.primaryViewConfigurationType = app->ViewportConfig.viewConfigurationType;

            XrResult result;
            OXR(result = xrBeginSession(app->Session, &sessionBeginInfo));

            app->SessionActive = (result == XR_SUCCESS);

            // Set session state once we have entered VR mode and have a valid session object.
            if (app->SessionActive) {
                XrPerfSettingsLevelEXT cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                switch (app->CpuLevel) {
                case 0:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
                    break;
                case 1:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
                    break;
                case 2:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                    break;
                case 3:
                    cpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
                    break;
                default:
                    ALOGE("Invalid CPU level %d", app->CpuLevel);
                    break;
                }

                XrPerfSettingsLevelEXT gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                switch (app->GpuLevel) {
                case 0:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
                    break;
                case 1:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
                    break;
                case 2:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
                    break;
                case 3:
                    gpuPerfLevel = XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
                    break;
                default:
                    ALOGE("Invalid GPU level %d", app->GpuLevel);
                    break;
                }

                PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
                OXR(xrGetInstanceProcAddr(
                    app->Instance, "xrPerfSettingsSetPerformanceLevelEXT",
                    (PFN_xrVoidFunction *)(&pfnPerfSettingsSetPerformanceLevelEXT)));

                OXR(pfnPerfSettingsSetPerformanceLevelEXT(app->Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, cpuPerfLevel));
                OXR(pfnPerfSettingsSetPerformanceLevelEXT(app->Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, gpuPerfLevel));

                PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
                OXR(xrGetInstanceProcAddr(
                    app->Instance, "xrSetAndroidApplicationThreadKHR",
                    (PFN_xrVoidFunction *)(&pfnSetAndroidApplicationThreadKHR)));

                OXR(pfnSetAndroidApplicationThreadKHR(
                    app->Session, XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, app->MainThreadTid));
                OXR(pfnSetAndroidApplicationThreadKHR(
                    app->Session, XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, app->RenderThreadTid));
            }
        }
        else if (state == XR_SESSION_STATE_STOPPING) {
            assert(app->Resumed == false);
            assert(app->SessionActive);

            OXR(xrEndSession(app->Session));
            app->SessionActive = false;
        }
    }

    static void ovrApp_HandleXrEvents(ovrApp *app) {
        XrEventDataBuffer eventDataBuffer = {};

        // Poll for events
        for (;;) {
            XrEventDataBaseHeader *baseEventHeader = (XrEventDataBaseHeader *)(&eventDataBuffer);
            baseEventHeader->type = XR_TYPE_EVENT_DATA_BUFFER;
            baseEventHeader->next = NULL;
            XrResult r;
            OXR(r = xrPollEvent(app->Instance, &eventDataBuffer));
            if (r != XR_SUCCESS) {
                break;
            }

            switch (baseEventHeader->type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_EVENTS_LOST event");
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                const XrEventDataInstanceLossPending *instance_loss_pending_event =
                    (XrEventDataInstanceLossPending *)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING event: time %f",
                    FromXrTime(instance_loss_pending_event->lossTime));
            } break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event");
                break;
            case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
                const XrEventDataPerfSettingsEXT *perf_settings_event = (XrEventDataPerfSettingsEXT *)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT event: type %d subdomain %d : level %d "
                    "-> "
                    "level %d",
                    perf_settings_event->type, perf_settings_event->subDomain, perf_settings_event->fromLevel,
                    perf_settings_event->toLevel);
            } break;
            case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB: {
                const XrEventDataDisplayRefreshRateChangedFB *refresh_rate_changed_event =
                    (XrEventDataDisplayRefreshRateChangedFB *)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB event: fromRate %f -> "
                    "toRate "
                    "%f",
                    refresh_rate_changed_event->fromDisplayRefreshRate,
                    refresh_rate_changed_event->toDisplayRefreshRate);
            } break;
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                XrEventDataReferenceSpaceChangePending *ref_space_change_event =
                    (XrEventDataReferenceSpaceChangePending *)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING event: changed space: %d "
                    "for "
                    "session %p at time %f",
                    ref_space_change_event->referenceSpaceType, (void *)ref_space_change_event->session,
                    FromXrTime(ref_space_change_event->changeTime));
            } break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const XrEventDataSessionStateChanged *session_state_changed_event =
                    (XrEventDataSessionStateChanged *)(baseEventHeader);
                ALOGV(
                    "xrPollEvent: received XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: %d for session %p at time %f",
                    session_state_changed_event->state, (void *)session_state_changed_event->session,
                    FromXrTime(session_state_changed_event->time));

                switch (session_state_changed_event->state) {
                case XR_SESSION_STATE_FOCUSED:
                    app->Focused = true;
                    break;
                case XR_SESSION_STATE_VISIBLE:
                    app->Focused = false;
                    break;
                case XR_SESSION_STATE_READY:
                case XR_SESSION_STATE_STOPPING:
                    ovrApp_HandleSessionStateChanges(app, session_state_changed_event->state);
                    break;
                default:
                    break;
                }
            } break;
            default:
                ALOGV("xrPollEvent: Unknown event");
                break;
            }
        }
    }
}