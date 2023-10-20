#pragma once
#include "common/Singleton.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android_native_app_glue.h>
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include "engine/types/LocVel.h"
#include <openxr/openxr_platform.h>

namespace nar
{
    struct ovrApp;

    class GameLoop : public Singleton<GameLoop> {
      public:
        void Run(android_app *app);

      private:
        XrAction CreateAction(
            XrActionSet actionSet, XrActionType type, const char *actionName, const char *localizedName,
            int countSubactionPaths, XrPath *subactionPaths);

        XrActionSuggestedBinding ActionSuggestedBinding(XrAction action, const char *bindingString);

        XrSpace CreateActionSpace(XrAction poseAction, XrPath subactionPath);

        XrActionStateBoolean GetActionStateBoolean(XrAction action);

        XrActionStateFloat GetActionStateFloat(XrAction action);

        XrActionStateVector2f GetActionStateVector2(XrAction action);

        XrActionSet CreateActionSet(int priority, const char *name, const char *localizedName);

        void UpdateStageBounds(ovrApp *pappState);

        bool ActionPoseIsActive(XrAction action, XrPath subactionPath);

        LocVel GetSpaceLocVel(XrSpace space, XrTime time);

        XrSpace leftControllerAimSpace = XR_NULL_HANDLE;
        XrSpace rightControllerAimSpace = XR_NULL_HANDLE;
        XrSpace leftControllerGripSpace = XR_NULL_HANDLE;
        XrSpace rightControllerGripSpace = XR_NULL_HANDLE;
    };
}