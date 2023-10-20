#pragma once
#include "ovrCompositorLayer_Union.h"
#include "ovrScene.h"
#include "engine/Constants.h"
#include "ovrRenderer.h"
#include "ovrEgl.h"

namespace nar
{
    struct ovrApp {
        ovrEgl Egl;
        ANativeWindow *NativeWindow;
        bool Resumed;
        bool Focused;

        XrInstance Instance;
        XrSession Session;
        XrViewConfigurationProperties ViewportConfig;
        XrViewConfigurationView ViewConfigurationView[ovrMaxNumEyes];
        XrSystemId SystemId;
        XrSpace HeadSpace;
        XrSpace LocalSpace;
        XrSpace StageSpace;
        XrSpace FakeStageSpace;
        XrSpace CurrentSpace;
        bool SessionActive;

        ovrScene Scene;

        float *SupportedDisplayRefreshRates;
        uint32_t RequestedDisplayRefreshRateIndex;
        uint32_t NumSupportedDisplayRefreshRates;
        PFN_xrGetDisplayRefreshRateFB pfnGetDisplayRefreshRate;
        PFN_xrRequestDisplayRefreshRateFB pfnRequestDisplayRefreshRate;

        int SwapInterval;
        int CpuLevel;
        int GpuLevel;
        // These threads will be marked as performance threads.
        int MainThreadTid;
        int RenderThreadTid;
        ovrCompositorLayer_Union Layers[ovrMaxLayerCount];
        int LayerCount;

        bool TouchPadDownLastFrame;
        ovrRenderer Renderer;
    };
}