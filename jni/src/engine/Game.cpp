#include "Game.h"
#include "vr_engine/GameLoop.h"
#include <android_native_app_glue.h>
#include "Global.h"
#include "vr_engine/OpenXRUtilityFunctions.h"
#include "vr_engine/OvrApp_GetInstance.h"
#include "vr_engine/OvrRenderer.h"
#include "vr_engine/OvrEgl.h"
#include "vr_engine/OvrApp.h"

namespace nar
{
    void Game::Run(android_app *app) {
        GameLoop::Get()->Run(app);
        Cleanup();
    }

    void Game::Cleanup() {
        ovrRenderer_Destroy(&appState.Renderer);

        free(projections);

        ovrScene_Destroy(&appState.Scene);
        ovrEgl_DestroyContext(&appState.Egl);

        OXR(xrDestroySpace(appState.HeadSpace));
        OXR(xrDestroySpace(appState.LocalSpace));
        // StageSpace is optional.
        if (appState.StageSpace != XR_NULL_HANDLE) {
            OXR(xrDestroySpace(appState.StageSpace));
        }
        OXR(xrDestroySpace(appState.FakeStageSpace));
        appState.CurrentSpace = XR_NULL_HANDLE;
        OXR(xrDestroySession(appState.Session));
        OXR(xrDestroyInstance(appState.Instance));

        ovrApp_Destroy(&appState);
    }
}