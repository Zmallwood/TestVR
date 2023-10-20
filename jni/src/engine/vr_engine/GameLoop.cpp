#include "GameLoop.h"
#include "engine/NativeActivity.h"
#include <sys/prctl.h>
#include "engine/Macros.h"
#include <unistd.h>
#include "engine/JNIEXPORTs.h"

namespace nar
{
    void GameLoop::Run(android_app *app) {
        JNIEnv *Env;
        app->activity->vm->AttachCurrentThread(&Env, NULL);

        // Note that AttachCurrentThread will reset the thread name.
        prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

        ovrApp_Clear(&appState);

        PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
        xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&xrInitializeLoaderKHR);
        if (xrInitializeLoaderKHR != NULL) {
            XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
            memset(&loaderInitializeInfoAndroid, 0, sizeof(loaderInitializeInfoAndroid));
            loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
            loaderInitializeInfoAndroid.next = NULL;
            loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
            loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;
            xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&loaderInitializeInfoAndroid);
        }

        jclass cls = Env->GetObjectClass(app->activity->clazz);
        draw_method = Env->GetMethodID(cls, "_draw", "(II[FI)V");
        setup_method = Env->GetMethodID(cls, "_setup", "()I");
        simulate_method = Env->GetMethodID(
            cls, "_simulate", "(DZZZZZZZZZZZZZZZZZZZZZZZFZFZFZFZFFZFFZFFFFFFFZFFFFFFFZFFFFFFFZFFFFFFF)V");

        // Log available layers.
        {
            XrResult result;

            PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
            OXR(result = xrGetInstanceProcAddr(
                    XR_NULL_HANDLE, "xrEnumerateApiLayerProperties",
                    (PFN_xrVoidFunction *)&xrEnumerateApiLayerProperties));
            if (result != XR_SUCCESS) {
                ALOGE("Failed to get xrEnumerateApiLayerProperties function pointer.");
                exit(1);
            }

            uint32_t numInputLayers = 0;
            uint32_t numOutputLayers = 0;
            OXR(xrEnumerateApiLayerProperties(numInputLayers, &numOutputLayers, NULL));

            numInputLayers = numOutputLayers;

            XrApiLayerProperties *layerProperties =
                (XrApiLayerProperties *)malloc(numOutputLayers * sizeof(XrApiLayerProperties));

            for (uint32_t i = 0; i < numOutputLayers; i++) {
                layerProperties[i].type = XR_TYPE_API_LAYER_PROPERTIES;
                layerProperties[i].next = NULL;
            }

            OXR(xrEnumerateApiLayerProperties(numInputLayers, &numOutputLayers, layerProperties));

            for (uint32_t i = 0; i < numOutputLayers; i++) {
                ALOGV("Found layer %s", layerProperties[i].layerName);
            }

            free(layerProperties);
        }

        // Check that the extensions required are present.
        const char *const requiredExtensionNames[] = {
            XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
            XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
            XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
            XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME,
            XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME,
            XR_KHR_COMPOSITION_LAYER_EQUIRECT2_EXTENSION_NAME,
            XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
            XR_FB_COLOR_SPACE_EXTENSION_NAME,
            XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME,
            XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME,
            XR_FB_FOVEATION_EXTENSION_NAME,
            XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME};
        const uint32_t numRequiredExtensions = sizeof(requiredExtensionNames) / sizeof(requiredExtensionNames[0]);

        // Check the list of required extensions against what is supported by the runtime.
        {
            XrResult result;
            PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
            OXR(result = xrGetInstanceProcAddr(
                    XR_NULL_HANDLE, "xrEnumerateInstanceExtensionProperties",
                    (PFN_xrVoidFunction *)&xrEnumerateInstanceExtensionProperties));
            if (result != XR_SUCCESS) {
                ALOGE("Failed to get xrEnumerateInstanceExtensionProperties function pointer.");
                exit(1);
            }

            uint32_t numInputExtensions = 0;
            uint32_t numOutputExtensions = 0;
            OXR(xrEnumerateInstanceExtensionProperties(NULL, numInputExtensions, &numOutputExtensions, NULL));
            ALOGV("xrEnumerateInstanceExtensionProperties found %u extension(s).", numOutputExtensions);

            numInputExtensions = numOutputExtensions;

            XrExtensionProperties *extensionProperties =
                (XrExtensionProperties *)malloc(numOutputExtensions * sizeof(XrExtensionProperties));

            for (uint32_t i = 0; i < numOutputExtensions; i++) {
                extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
                extensionProperties[i].next = NULL;
            }

            OXR(xrEnumerateInstanceExtensionProperties(
                NULL, numInputExtensions, &numOutputExtensions, extensionProperties));
            for (uint32_t i = 0; i < numOutputExtensions; i++) {
                ALOGV("Extension #%d = '%s'.", i, extensionProperties[i].extensionName);
            }

            for (uint32_t i = 0; i < numRequiredExtensions; i++) {
                bool found = false;
                for (uint32_t j = 0; j < numOutputExtensions; j++) {
                    if (!strcmp(requiredExtensionNames[i], extensionProperties[j].extensionName)) {
                        ALOGV("Found required extension %s", requiredExtensionNames[i]);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ALOGE("Failed to find required extension %s", requiredExtensionNames[i]);
                    exit(1);
                }
            }

            free(extensionProperties);
        }

        // Create the OpenXR instance.
        XrApplicationInfo appInfo;
        memset(&appInfo, 0, sizeof(appInfo));
        strcpy(appInfo.applicationName, "OpenXR_NativeActivity");
        appInfo.applicationVersion = 0;
        strcpy(appInfo.engineName, "Oculus Mobile Sample");
        appInfo.engineVersion = 0;
        appInfo.apiVersion = XR_CURRENT_API_VERSION;

        XrInstanceCreateInfo instanceCreateInfo;
        memset(&instanceCreateInfo, 0, sizeof(instanceCreateInfo));
        instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.next = NULL;
        instanceCreateInfo.createFlags = 0;
        instanceCreateInfo.applicationInfo = appInfo;
        instanceCreateInfo.enabledApiLayerCount = 0;
        instanceCreateInfo.enabledApiLayerNames = NULL;
        instanceCreateInfo.enabledExtensionCount = numRequiredExtensions;
        instanceCreateInfo.enabledExtensionNames = requiredExtensionNames;

        XrResult initResult;
        OXR(initResult = xrCreateInstance(&instanceCreateInfo, &appState.Instance));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to create XR instance: %d.", initResult);
            exit(1);
        }

        XrInstanceProperties instanceInfo;
        instanceInfo.type = XR_TYPE_INSTANCE_PROPERTIES;
        instanceInfo.next = NULL;
        OXR(xrGetInstanceProperties(appState.Instance, &instanceInfo));
        ALOGV(
            "Runtime %s: Version : %u.%u.%u", instanceInfo.runtimeName, XR_VERSION_MAJOR(instanceInfo.runtimeVersion),
            XR_VERSION_MINOR(instanceInfo.runtimeVersion), XR_VERSION_PATCH(instanceInfo.runtimeVersion));

        XrSystemGetInfo systemGetInfo;
        memset(&systemGetInfo, 0, sizeof(systemGetInfo));
        systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
        systemGetInfo.next = NULL;
        systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrSystemId systemId;
        OXR(initResult = xrGetSystem(appState.Instance, &systemGetInfo, &systemId));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to get system.");
            exit(1);
        }

        XrSystemColorSpacePropertiesFB colorSpacePropertiesFB = {};
        colorSpacePropertiesFB.type = XR_TYPE_SYSTEM_COLOR_SPACE_PROPERTIES_FB;

        XrSystemProperties systemProperties = {};
        systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
        systemProperties.next = &colorSpacePropertiesFB;
        OXR(xrGetSystemProperties(appState.Instance, systemId, &systemProperties));

        ALOGV("System Properties: Name=%s VendorId=%x", systemProperties.systemName, systemProperties.vendorId);
        ALOGV(
            "System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
            systemProperties.graphicsProperties.maxSwapchainImageWidth,
            systemProperties.graphicsProperties.maxSwapchainImageHeight,
            systemProperties.graphicsProperties.maxLayerCount);
        ALOGV(
            "System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
            systemProperties.trackingProperties.orientationTracking ? "True" : "False",
            systemProperties.trackingProperties.positionTracking ? "True" : "False");

        ALOGV("System Color Space Properties: colorspace=%d", colorSpacePropertiesFB.colorSpace);

        assert(ovrMaxLayerCount <= systemProperties.graphicsProperties.maxLayerCount);

        // Get the graphics requirements.
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
        OXR(xrGetInstanceProcAddr(
            appState.Instance, "xrGetOpenGLESGraphicsRequirementsKHR",
            (PFN_xrVoidFunction *)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
        graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
        OXR(pfnGetOpenGLESGraphicsRequirementsKHR(appState.Instance, systemId, &graphicsRequirements));

        // Create the EGL Context
        ovrEgl_CreateContext(&appState.Egl, NULL);

        // Check the graphics requirements.
        int eglMajor = 0;
        int eglMinor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &eglMajor);
        glGetIntegerv(GL_MINOR_VERSION, &eglMinor);
        const XrVersion eglVersion = XR_MAKE_VERSION(eglMajor, eglMinor, 0);
        if (eglVersion < graphicsRequirements.minApiVersionSupported ||
            eglVersion > graphicsRequirements.maxApiVersionSupported) {
            ALOGE("GLES version %d.%d not supported", eglMajor, eglMinor);
            exit(0);
        }

        appState.CpuLevel = CPU_LEVEL;
        appState.GpuLevel = GPU_LEVEL;
        appState.MainThreadTid = gettid();

        appState.SystemId = systemId;

        // Create the OpenXR Session.
        XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingAndroidGLES = {};
        graphicsBindingAndroidGLES.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
        graphicsBindingAndroidGLES.next = NULL;
        graphicsBindingAndroidGLES.display = appState.Egl.Display;
        graphicsBindingAndroidGLES.config = appState.Egl.Config;
        graphicsBindingAndroidGLES.context = appState.Egl.Context;

        XrSessionCreateInfo sessionCreateInfo = {};
        memset(&sessionCreateInfo, 0, sizeof(sessionCreateInfo));
        sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
        sessionCreateInfo.next = &graphicsBindingAndroidGLES;
        sessionCreateInfo.createFlags = 0;
        sessionCreateInfo.systemId = appState.SystemId;

        OXR(initResult = xrCreateSession(appState.Instance, &sessionCreateInfo, &appState.Session));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to create XR session: %d.", initResult);
            exit(1);
        }

        // App only supports the primary stereo view config.
        const XrViewConfigurationType supportedViewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

        // Enumerate the viewport configurations.
        uint32_t viewportConfigTypeCount = 0;
        OXR(xrEnumerateViewConfigurations(appState.Instance, appState.SystemId, 0, &viewportConfigTypeCount, NULL));

        XrViewConfigurationType *viewportConfigurationTypes =
            (XrViewConfigurationType *)malloc(viewportConfigTypeCount * sizeof(XrViewConfigurationType));

        OXR(xrEnumerateViewConfigurations(
            appState.Instance, appState.SystemId, viewportConfigTypeCount, &viewportConfigTypeCount,
            viewportConfigurationTypes));

        ALOGV("Available Viewport Configuration Types: %d", viewportConfigTypeCount);

        for (uint32_t i = 0; i < viewportConfigTypeCount; i++) {
            const XrViewConfigurationType viewportConfigType = viewportConfigurationTypes[i];

            ALOGV(
                "Viewport configuration type %d : %s", viewportConfigType,
                viewportConfigType == supportedViewConfigType ? "Selected" : "");

            XrViewConfigurationProperties viewportConfig;
            viewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
            OXR(xrGetViewConfigurationProperties(
                appState.Instance, appState.SystemId, viewportConfigType, &viewportConfig));
            ALOGV(
                "FovMutable=%s ConfigurationType %d", viewportConfig.fovMutable ? "true" : "false",
                viewportConfig.viewConfigurationType);

            uint32_t viewCount;
            OXR(xrEnumerateViewConfigurationViews(
                appState.Instance, appState.SystemId, viewportConfigType, 0, &viewCount, NULL));

            if (viewCount > 0) {
                XrViewConfigurationView *elements =
                    (XrViewConfigurationView *)malloc(viewCount * sizeof(XrViewConfigurationView));

                for (uint32_t e = 0; e < viewCount; e++) {
                    elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                    elements[e].next = NULL;
                }

                OXR(xrEnumerateViewConfigurationViews(
                    appState.Instance, appState.SystemId, viewportConfigType, viewCount, &viewCount, elements));

                // Log the view config info for each view type for debugging purposes.
                for (uint32_t e = 0; e < viewCount; e++) {
                    const XrViewConfigurationView *element = &elements[e];

                    ALOGV(
                        "Viewport [%d]: Recommended Width=%d Height=%d SampleCount=%d", e,
                        element->recommendedImageRectWidth, element->recommendedImageRectHeight,
                        element->recommendedSwapchainSampleCount);

                    ALOGV(
                        "Viewport [%d]: Max Width=%d Height=%d SampleCount=%d", e, element->maxImageRectWidth,
                        element->maxImageRectHeight, element->maxSwapchainSampleCount);
                }

                // Cache the view config properties for the selected config type.
                if (viewportConfigType == supportedViewConfigType) {
                    assert(viewCount == ovrMaxNumEyes);
                    for (uint32_t e = 0; e < viewCount; e++) {
                        appState.ViewConfigurationView[e] = elements[e];
                    }
                }

                free(elements);
            }
            else {
                ALOGE("Empty viewport configuration type: %d", viewCount);
            }
        }

        free(viewportConfigurationTypes);

        // Get the viewport configuration info for the chosen viewport configuration type.
        appState.ViewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;

        OXR(xrGetViewConfigurationProperties(
            appState.Instance, appState.SystemId, supportedViewConfigType, &appState.ViewportConfig));

        // Enumerate the supported color space options for the system.
        {
            PFN_xrEnumerateColorSpacesFB pfnxrEnumerateColorSpacesFB = NULL;
            OXR(xrGetInstanceProcAddr(
                appState.Instance, "xrEnumerateColorSpacesFB", (PFN_xrVoidFunction *)(&pfnxrEnumerateColorSpacesFB)));

            uint32_t colorSpaceCountOutput = 0;
            OXR(pfnxrEnumerateColorSpacesFB(appState.Session, 0, &colorSpaceCountOutput, NULL));

            XrColorSpaceFB *colorSpaces = (XrColorSpaceFB *)malloc(colorSpaceCountOutput * sizeof(XrColorSpaceFB));

            OXR(pfnxrEnumerateColorSpacesFB(
                appState.Session, colorSpaceCountOutput, &colorSpaceCountOutput, colorSpaces));
            ALOGV("Supported ColorSpaces:");

            for (uint32_t i = 0; i < colorSpaceCountOutput; i++) {
                ALOGV("%d:%d", i, colorSpaces[i]);
            }

            const XrColorSpaceFB requestColorSpace = XR_COLOR_SPACE_REC2020_FB;

            PFN_xrSetColorSpaceFB pfnxrSetColorSpaceFB = NULL;
            OXR(xrGetInstanceProcAddr(
                appState.Instance, "xrSetColorSpaceFB", (PFN_xrVoidFunction *)(&pfnxrSetColorSpaceFB)));

            OXR(pfnxrSetColorSpaceFB(appState.Session, requestColorSpace));

            free(colorSpaces);
        }

        // Get the supported display refresh rates for the system.
        {
            PFN_xrEnumerateDisplayRefreshRatesFB pfnxrEnumerateDisplayRefreshRatesFB = NULL;
            OXR(xrGetInstanceProcAddr(
                appState.Instance, "xrEnumerateDisplayRefreshRatesFB",
                (PFN_xrVoidFunction *)(&pfnxrEnumerateDisplayRefreshRatesFB)));

            OXR(pfnxrEnumerateDisplayRefreshRatesFB(
                appState.Session, 0, &appState.NumSupportedDisplayRefreshRates, NULL));

            appState.SupportedDisplayRefreshRates =
                (float *)malloc(appState.NumSupportedDisplayRefreshRates * sizeof(float));
            OXR(pfnxrEnumerateDisplayRefreshRatesFB(
                appState.Session, appState.NumSupportedDisplayRefreshRates, &appState.NumSupportedDisplayRefreshRates,
                appState.SupportedDisplayRefreshRates));
            ALOGV("Supported Refresh Rates:");
            for (uint32_t i = 0; i < appState.NumSupportedDisplayRefreshRates; i++) {
                ALOGV("%d:%f", i, appState.SupportedDisplayRefreshRates[i]);
            }

            OXR(xrGetInstanceProcAddr(
                appState.Instance, "xrGetDisplayRefreshRateFB",
                (PFN_xrVoidFunction *)(&appState.pfnGetDisplayRefreshRate)));

            float currentDisplayRefreshRate = 0.0f;
            OXR(appState.pfnGetDisplayRefreshRate(appState.Session, &currentDisplayRefreshRate));
            ALOGV("Current System Display Refresh Rate: %f", currentDisplayRefreshRate);

            OXR(xrGetInstanceProcAddr(
                appState.Instance, "xrRequestDisplayRefreshRateFB",
                (PFN_xrVoidFunction *)(&appState.pfnRequestDisplayRefreshRate)));

            // Test requesting the system default.
            OXR(appState.pfnRequestDisplayRefreshRate(appState.Session, 0.0f));
            ALOGV("Requesting system default display refresh rate");
        }

        bool stageSupported = false;

        uint32_t numOutputSpaces = 0;
        OXR(xrEnumerateReferenceSpaces(appState.Session, 0, &numOutputSpaces, NULL));

        XrReferenceSpaceType *referenceSpaces =
            (XrReferenceSpaceType *)malloc(numOutputSpaces * sizeof(XrReferenceSpaceType));

        OXR(xrEnumerateReferenceSpaces(appState.Session, numOutputSpaces, &numOutputSpaces, referenceSpaces));

        for (uint32_t i = 0; i < numOutputSpaces; i++) {
            if (referenceSpaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
                stageSupported = true;
                break;
            }
        }

        free(referenceSpaces);

        // Create a space to the first path
        XrReferenceSpaceCreateInfo spaceCreateInfo = {};
        spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
        OXR(xrCreateReferenceSpace(appState.Session, &spaceCreateInfo, &appState.HeadSpace));

        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        OXR(xrCreateReferenceSpace(appState.Session, &spaceCreateInfo, &appState.LocalSpace));

        // Create a default stage space to use if SPACE_TYPE_STAGE is not
        // supported, or calls to xrGetReferenceSpaceBoundsRect fail.
        {
            spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            spaceCreateInfo.poseInReferenceSpace.position.y = -1.6750f;
            OXR(xrCreateReferenceSpace(appState.Session, &spaceCreateInfo, &appState.FakeStageSpace));
            ALOGV("Created fake stage space from local space with offset");
            appState.CurrentSpace = appState.FakeStageSpace;
        }

        if (stageSupported) {
            spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
            spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
            OXR(xrCreateReferenceSpace(appState.Session, &spaceCreateInfo, &appState.StageSpace));
            ALOGV("Created stage space");
            appState.CurrentSpace = appState.StageSpace;
        }

        for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
            memset(&projections[eye], 0, sizeof(XrView));
            projections[eye].type = XR_TYPE_VIEW;
        }

        // Actions
        XrActionSet runningActionSet = CreateActionSet(1, "running_action_set", "Action Set used on main loop");
        XrAction toggleActionLeftTrigger = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_toggle_trigger", "Left Toggle Trigger", 0, NULL);
        XrAction toggleActionLeftX =
            CreateAction(runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_toggle_x", "Left Toggle X", 0, NULL);
        XrAction toggleActionLeftY =
            CreateAction(runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_toggle_y", "Left ToggleY ", 0, NULL);
        XrAction toggleActionMenu = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_toggle_menu", "Left Toggle Menu ", 0, NULL);
        XrAction toggleActionLeftSqueeze = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_toggle_squeeze", "Left Toggle Squeeze", 0, NULL);

        XrAction toggleActionRightTrigger = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "right_toggle_trigger", "Right Toggle Trigger", 0, NULL);
        XrAction toggleActionRightA =
            CreateAction(runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "right_toggle_a", "Right Toggle A", 0, NULL);
        XrAction toggleActionRightB =
            CreateAction(runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "right_toggle_b", "Right Toggle B", 0, NULL);
        XrAction toggleActionRightSqueeze = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "right_toggle_squeeze", "Right Toggle Squeeze", 0, NULL);

        XrAction moveOnXActionLeftSqueeze =
            CreateAction(runningActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "left_move_on_x", "Left Move on X", 0, NULL);
        XrAction moveOnXActionRightSqueeze =
            CreateAction(runningActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "right_move_on_x", "Right Move on X", 0, NULL);

        XrAction moveOnYActionLeftTriggerValue =
            CreateAction(runningActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "left_move_on_y", "Left Move on Y", 0, NULL);
        XrAction moveOnYActionRightTriggerValue =
            CreateAction(runningActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "right_move_on_y", "Right Move on Y", 0, NULL);

        XrAction moveOnJoystickActionLeft = CreateAction(
            runningActionSet, XR_ACTION_TYPE_VECTOR2F_INPUT, "left_move_on_joy", "Left Move on Joy", 0, NULL);
        XrAction moveOnJoystickActionRight = CreateAction(
            runningActionSet, XR_ACTION_TYPE_VECTOR2F_INPUT, "right_move_on_joy", "Right Move on Joy", 0, NULL);

        XrAction thumbstickClickActionLeft = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "left_thumbstick_click", "Left Thumbstick Click", 0, NULL);
        XrAction thumbstickClickActionRight = CreateAction(
            runningActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "right_thumbstick_click", "Right Thumbstick Click", 0,
            NULL);

        vibrateLeftFeedback = CreateAction(
            runningActionSet, XR_ACTION_TYPE_VIBRATION_OUTPUT, "vibrate_left_feedback",
            "Vibrate Left Controller Feedback", 0, NULL);
        vibrateRightFeedback = CreateAction(
            runningActionSet, XR_ACTION_TYPE_VIBRATION_OUTPUT, "vibrate_right_feedback",
            "Vibrate Right Controller Feedback", 0, NULL);

        XrPath leftHandPath;
        OXR(xrStringToPath(appState.Instance, "/user/hand/left", &leftHandPath));
        XrPath rightHandPath;
        OXR(xrStringToPath(appState.Instance, "/user/hand/right", &rightHandPath));
        XrPath handSubactionPaths[2] = {leftHandPath, rightHandPath};

        XrAction aimPoseActionLeft =
            CreateAction(runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "left_aim_pose", NULL, 2, handSubactionPaths);
        XrAction aimPoseActionRight =
            CreateAction(runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "right_aim_pose", NULL, 2, handSubactionPaths);
        XrAction gripPoseActionLeft =
            CreateAction(runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "left_grip_pose", NULL, 2, handSubactionPaths);
        XrAction gripPoseActionRight =
            CreateAction(runningActionSet, XR_ACTION_TYPE_POSE_INPUT, "right_grip_pose", NULL, 2, handSubactionPaths);

        XrPath interactionProfilePath = XR_NULL_PATH;
        XrPath interactionProfilePathTouch = XR_NULL_PATH;
        XrPath interactionProfilePathKHRSimple = XR_NULL_PATH;

        OXR(xrStringToPath(
            appState.Instance, "/interaction_profiles/oculus/touch_controller", &interactionProfilePathTouch));
        OXR(xrStringToPath(
            appState.Instance, "/interaction_profiles/khr/simple_controller", &interactionProfilePathKHRSimple));

        // Toggle this to force simple as a first choice, otherwise use it as a last resort
        bool useSimpleProfile = false; /// true;
        if (useSimpleProfile) {
            ALOGV("xrSuggestInteractionProfileBindings found bindings for Khronos SIMPLE controller");
            interactionProfilePath = interactionProfilePathKHRSimple;
        }
        else {
            // Query Set
            XrActionSet queryActionSet = CreateActionSet(1, "query_action_set", "Action Set used to query device caps");
            XrAction dummyAction =
                CreateAction(queryActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "dummy_action", "Dummy Action", 0, NULL);

            // Map bindings
            XrActionSuggestedBinding bindings[1];
            int currBinding = 0;
            bindings[currBinding++] = ActionSuggestedBinding(dummyAction, "/user/hand/right/input/system/click");

            XrInteractionProfileSuggestedBinding suggestedBindings = {};
            suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
            suggestedBindings.next = NULL;
            suggestedBindings.suggestedBindings = bindings;
            suggestedBindings.countSuggestedBindings = currBinding;

            // Try all
            suggestedBindings.interactionProfile = interactionProfilePathTouch;
            XrResult suggestTouchResult = xrSuggestInteractionProfileBindings(appState.Instance, &suggestedBindings);
            OXR(suggestTouchResult);

            if (XR_SUCCESS == suggestTouchResult) {
                ALOGV("xrSuggestInteractionProfileBindings found bindings for QUEST controller");
                interactionProfilePath = interactionProfilePathTouch;
            }

            if (interactionProfilePath == XR_NULL_PATH) {
                // Simple as a fallback
                bindings[0] = ActionSuggestedBinding(dummyAction, "/user/hand/right/input/select/click");
                suggestedBindings.interactionProfile = interactionProfilePathKHRSimple;
                XrResult suggestKHRSimpleResult =
                    xrSuggestInteractionProfileBindings(appState.Instance, &suggestedBindings);
                OXR(suggestKHRSimpleResult);
                if (XR_SUCCESS == suggestKHRSimpleResult) {
                    ALOGV("xrSuggestInteractionProfileBindings found bindings for Khronos SIMPLE controller");
                    interactionProfilePath = interactionProfilePathKHRSimple;
                }
                else {
                    ALOGE("xrSuggestInteractionProfileBindings did NOT find any bindings.");
                    assert(false);
                }
            }
        }

        // Action creation
        {
            // Map bindings

            XrActionSuggestedBinding bindings[22]; // large enough for all profiles
            int currBinding = 0;

            {
                if (interactionProfilePath == interactionProfilePathTouch) {
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionLeftTrigger, "/user/hand/left/input/trigger");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionRightTrigger, "/user/hand/right/input/trigger");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionLeftSqueeze, "/user/hand/left/input/squeeze");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionRightSqueeze, "/user/hand/right/input/squeeze");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionLeftX, "/user/hand/left/input/x/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionRightA, "/user/hand/right/input/a/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnXActionLeftSqueeze, "/user/hand/left/input/squeeze/value");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnXActionRightSqueeze, "/user/hand/right/input/squeeze/value");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnYActionLeftTriggerValue, "/user/hand/left/input/trigger/value");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnYActionRightTriggerValue, "/user/hand/right/input/trigger/value");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnJoystickActionLeft, "/user/hand/left/input/thumbstick");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(moveOnJoystickActionRight, "/user/hand/right/input/thumbstick");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(thumbstickClickActionLeft, "/user/hand/left/input/thumbstick/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(thumbstickClickActionRight, "/user/hand/right/input/thumbstick/click");
                    bindings[currBinding++] = ActionSuggestedBinding(
                        /*vibrateLeftToggle*/ toggleActionLeftY, "/user/hand/left/input/y/click");
                    bindings[currBinding++] = ActionSuggestedBinding(
                        /*vibrateRightToggle*/ toggleActionRightB, "/user/hand/right/input/b/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleActionMenu, "/user/hand/left/input/menu/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateLeftFeedback, "/user/hand/left/output/haptic");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateRightFeedback, "/user/hand/right/output/haptic");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(aimPoseActionLeft, "/user/hand/left/input/aim/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(aimPoseActionRight, "/user/hand/right/input/aim/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(gripPoseActionLeft, "/user/hand/left/input/grip/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(gripPoseActionRight, "/user/hand/right/input/grip/pose");
                }

                /*if (interactionProfilePath == interactionProfilePathKHRSimple) {
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleAction, "/user/hand/left/input/select/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(toggleAction, "/user/hand/right/input/select/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateLeftToggle, "/user/hand/left/input/menu/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateRightToggle, "/user/hand/right/input/menu/click");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateLeftFeedback, "/user/hand/left/output/haptic");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(vibrateRightFeedback, "/user/hand/right/output/haptic");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(aimPoseAction, "/user/hand/left/input/aim/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(aimPoseAction, "/user/hand/right/input/aim/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(gripPoseAction, "/user/hand/left/input/grip/pose");
                    bindings[currBinding++] =
                        ActionSuggestedBinding(gripPoseAction, "/user/hand/right/input/grip/pose");
                }*/
            }

            XrInteractionProfileSuggestedBinding suggestedBindings = {};
            suggestedBindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
            suggestedBindings.next = NULL;
            suggestedBindings.interactionProfile = interactionProfilePath;
            suggestedBindings.suggestedBindings = bindings;
            suggestedBindings.countSuggestedBindings = currBinding;
            OXR(xrSuggestInteractionProfileBindings(appState.Instance, &suggestedBindings));

            // Attach to session
            XrSessionActionSetsAttachInfo attachInfo = {};
            attachInfo.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
            attachInfo.next = NULL;
            attachInfo.countActionSets = 1;
            attachInfo.actionSets = &runningActionSet;
            OXR(xrAttachSessionActionSets(appState.Session, &attachInfo));

            // Enumerate actions
            XrPath actionPathsBuffer[16];
            char stringBuffer[256];
            XrAction actionsToEnumerate[] = {
                toggleActionLeftTrigger,
                toggleActionRightTrigger,
                toggleActionLeftSqueeze,
                toggleActionRightSqueeze,
                toggleActionLeftX,
                toggleActionLeftY,
                toggleActionMenu,
                toggleActionRightA,
                toggleActionRightB,
                moveOnXActionLeftSqueeze,
                moveOnXActionRightSqueeze,
                moveOnYActionLeftTriggerValue,
                moveOnYActionRightTriggerValue,
                moveOnJoystickActionLeft,
                moveOnJoystickActionRight,
                thumbstickClickActionLeft,
                thumbstickClickActionRight,
                vibrateLeftFeedback,
                vibrateRightFeedback,
                aimPoseActionLeft,
                aimPoseActionRight,
                gripPoseActionLeft,
                gripPoseActionRight};
            for (size_t i = 0; i < sizeof(actionsToEnumerate) / sizeof(actionsToEnumerate[0]); ++i) {
                XrBoundSourcesForActionEnumerateInfo enumerateInfo = {};
                enumerateInfo.type = XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO;
                enumerateInfo.next = NULL;
                enumerateInfo.action = actionsToEnumerate[i];

                // Get Count
                uint32_t countOutput = 0;
                OXR(xrEnumerateBoundSourcesForAction(
                    appState.Session, &enumerateInfo, 0 /* request size */, &countOutput, NULL));
                ALOGV(
                    "xrEnumerateBoundSourcesForAction action=%lld count=%u", (long long)enumerateInfo.action,
                    countOutput);

                if (countOutput < 16) {
                    OXR(xrEnumerateBoundSourcesForAction(
                        appState.Session, &enumerateInfo, 16, &countOutput, actionPathsBuffer));
                    for (uint32_t a = 0; a < countOutput; ++a) {
                        XrInputSourceLocalizedNameGetInfo nameGetInfo = {};
                        nameGetInfo.type = XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO;
                        nameGetInfo.next = NULL;
                        nameGetInfo.sourcePath = actionPathsBuffer[a];
                        nameGetInfo.whichComponents = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                                                      XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                                      XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

                        uint32_t stringCount = 0u;
                        OXR(xrGetInputSourceLocalizedName(appState.Session, &nameGetInfo, 0, &stringCount, NULL));
                        if (stringCount < 256) {
                            OXR(xrGetInputSourceLocalizedName(
                                appState.Session, &nameGetInfo, 256, &stringCount, stringBuffer));
                            char pathStr[256];
                            uint32_t strLen = 0;
                            OXR(xrPathToString(
                                appState.Instance, actionPathsBuffer[a], (uint32_t)sizeof(pathStr), &strLen, pathStr));
                            ALOGV(
                                "  -> path = %lld `%s` -> `%s`", (long long)actionPathsBuffer[a], pathStr,
                                stringBuffer);
                        }
                    }
                }
            }
        }

        ovrRenderer_Create(
            appState.Session, &appState.Renderer, appState.ViewConfigurationView[0].recommendedImageRectWidth,
            appState.ViewConfigurationView[0].recommendedImageRectHeight);

        ovrRenderer_SetFoveation(
            &appState.Instance, &appState.Session, &appState.Renderer, XR_FOVEATION_LEVEL_HIGH_FB, 0,
            XR_FOVEATION_DYNAMIC_DISABLED_FB);

        app->userData = &appState;
        app->onAppCmd = app_handle_cmd;

        bool stageBoundsDirty = true;

        double startTime = -1.0;

        // App-specific input
        float appQuadPositionX = 0.0f;
        float appQuadPositionY = 0.0f;
        float appCylPositionX = 0.0f;
        float appCylPositionY = 0.0f;

        while (app->destroyRequested == 0) {
            // Read all pending events.
            for (;;) {
                int events;
                struct android_poll_source *source;
                // If the timeout is zero, returns immediately without blocking.
                // If the timeout is negative, waits indefinitely until an event appears.
                const int timeoutMilliseconds =
                    (appState.Resumed == false && appState.SessionActive == false && app->destroyRequested == 0) ? -1
                                                                                                                 : 0;
                if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void **)&source) < 0) {
                    break;
                }

                // Process this event.
                if (source != NULL) {
                    source->process(app, source);
                }
            }

            ovrApp_HandleXrEvents(&appState);

            if (appState.SessionActive == false) {
                continue;
            }

            if (leftControllerAimSpace == XR_NULL_HANDLE) {
                leftControllerAimSpace = CreateActionSpace(aimPoseActionLeft, leftHandPath);
            }
            if (rightControllerAimSpace == XR_NULL_HANDLE) {
                rightControllerAimSpace = CreateActionSpace(aimPoseActionRight, rightHandPath);
            }
            if (leftControllerGripSpace == XR_NULL_HANDLE) {
                leftControllerGripSpace = CreateActionSpace(gripPoseActionLeft, leftHandPath);
            }
            if (rightControllerGripSpace == XR_NULL_HANDLE) {
                rightControllerGripSpace = CreateActionSpace(gripPoseActionRight, rightHandPath);
            }

            // Create the scene if not yet created.
            // The scene is created here to be able to show a loading icon.
            if (!ovrScene_IsCreated(&appState.Scene)) {
                ovrScene_Create(app->activity->assetManager, appState.Instance, appState.Session, &appState.Scene);
                /*appState.Scene.SceneMatrices=*/Env->CallIntMethod(app->activity->clazz, setup_method);
            }

            if (stageBoundsDirty) {
                UpdateStageBounds(&appState);
                stageBoundsDirty = false;
            }

            // NOTE: OpenXR does not use the concept of frame indices. Instead,
            // XrWaitFrame returns the predicted display time.
            XrFrameWaitInfo waitFrameInfo = {};
            waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
            waitFrameInfo.next = NULL;

            XrFrameState frameState = {};
            frameState.type = XR_TYPE_FRAME_STATE;
            frameState.next = NULL;

            OXR(xrWaitFrame(appState.Session, &waitFrameInfo, &frameState));

            // Get the HMD pose, predicted for the middle of the time period during which
            // the new eye images will be displayed. The number of frames predicted ahead
            // depends on the pipeline depth of the engine and the synthesis rate.
            // The better the prediction, the less black will be pulled in at the edges.
            XrFrameBeginInfo beginFrameDesc = {};
            beginFrameDesc.type = XR_TYPE_FRAME_BEGIN_INFO;
            beginFrameDesc.next = NULL;
            OXR(xrBeginFrame(appState.Session, &beginFrameDesc));

            XrSpaceLocation loc = {};
            loc.type = XR_TYPE_SPACE_LOCATION;
            OXR(xrLocateSpace(appState.HeadSpace, appState.CurrentSpace, frameState.predictedDisplayTime, &loc));
            XrPosef xfStageFromHead = loc.pose;
            OXR(xrLocateSpace(appState.HeadSpace, appState.LocalSpace, frameState.predictedDisplayTime, &loc));

            XrViewLocateInfo projectionInfo = {};
            projectionInfo.type = XR_TYPE_VIEW_LOCATE_INFO;
            projectionInfo.viewConfigurationType = appState.ViewportConfig.viewConfigurationType;
            projectionInfo.displayTime = frameState.predictedDisplayTime;
            projectionInfo.space = appState.HeadSpace;

            XrViewState viewState = {XR_TYPE_VIEW_STATE, NULL};

            uint32_t projectionCapacityInput = ovrMaxNumEyes;
            uint32_t projectionCountOutput = projectionCapacityInput;

            OXR(xrLocateViews(
                appState.Session, &projectionInfo, &viewState, projectionCapacityInput, &projectionCountOutput,
                projections));
            //

            // Simple animation
            double timeInSeconds = FromXrTime(frameState.predictedDisplayTime);
            if (startTime < 0.0) {
                startTime = timeInSeconds;
            }
            timeInSeconds -= startTime;
            double intPart = 0.0;
            __attribute__((unused)) double fractTime = modf(timeInSeconds, &intPart);

            ovrSceneMatrices sceneMatrices;
            XrPosef viewTransform[2];

            for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
                XrPosef xfHeadFromEye = projections[eye].pose;
                XrPosef xfStageFromEye = XrPosef_Multiply(xfStageFromHead, xfHeadFromEye);
                viewTransform[eye] = XrPosef_Inverse(xfStageFromEye);

                sceneMatrices.ViewMatrix[eye] = XrMatrix4x4f_CreateFromRigidTransform(&viewTransform[eye]);

                const XrFovf fov = projections[eye].fov;
                XrMatrix4x4f_CreateProjectionFov(
                    &sceneMatrices.ProjectionMatrix[eye], GRAPHICS_OPENGL_ES, fov, 0.1f, 0.0f);
            }

            // update input information
            XrAction controller[] = {aimPoseActionLeft, gripPoseActionLeft, aimPoseActionRight, gripPoseActionRight};
            XrPath subactionPath[] = {leftHandPath, leftHandPath, rightHandPath, rightHandPath};
            XrSpace controllerSpace[] = {
                leftControllerAimSpace,
                leftControllerGripSpace,
                rightControllerAimSpace,
                rightControllerGripSpace,
            };
            for (int i = 0; i < 4; i++) {
                if (ActionPoseIsActive(controller[i], subactionPath[i])) {
                    LocVel lv = GetSpaceLocVel(controllerSpace[i], frameState.predictedDisplayTime);
                    appState.Scene.TrackedController[i].Active =
                        (lv.loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
                    appState.Scene.TrackedController[i].Pose = lv.loc.pose;
                    for (int j = 0; j < 3; j++) {
                        float dt = 0.01f; // use 0.2f for for testing velocity vectors
                        (&appState.Scene.TrackedController[i].Pose.position.x)[j] += (&lv.vel.linearVelocity.x)[j] * dt;
                    }
                }
                else {
                    ovrTrackedController_Clear(&appState.Scene.TrackedController[i]);
                }
            }

            // OpenXR input
            {
                // sync action data
                XrActiveActionSet activeActionSet = {};
                activeActionSet.actionSet = runningActionSet;
                activeActionSet.subactionPath = XR_NULL_PATH;

                XrActionsSyncInfo syncInfo = {};
                syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
                syncInfo.next = NULL;
                syncInfo.countActiveActionSets = 1;
                syncInfo.activeActionSets = &activeActionSet;
                OXR(xrSyncActions(appState.Session, &syncInfo));

                // query input action states
                XrActionStateGetInfo getInfo = {};
                getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
                getInfo.next = NULL;
                getInfo.subactionPath = XR_NULL_PATH;

                XrActionStateBoolean toggleStateLeftTrigger = GetActionStateBoolean(toggleActionLeftTrigger);
                XrActionStateBoolean toggleStateRightTrigger = GetActionStateBoolean(toggleActionRightTrigger);
                XrActionStateBoolean toggleStateLeftSqueeze = GetActionStateBoolean(toggleActionLeftSqueeze);
                XrActionStateBoolean toggleStateRightSqueeze = GetActionStateBoolean(toggleActionRightSqueeze);
                XrActionStateBoolean toggleStateLeftX = GetActionStateBoolean(toggleActionLeftX);
                XrActionStateBoolean toggleStateLeftY = GetActionStateBoolean(toggleActionLeftY);
                XrActionStateBoolean toggleStateMenu = GetActionStateBoolean(toggleActionMenu);
                XrActionStateBoolean toggleStateRightA = GetActionStateBoolean(toggleActionRightA);
                XrActionStateBoolean toggleStateRightB = GetActionStateBoolean(toggleActionRightB);

                XrActionStateBoolean thumbstickClickStateLeft = GetActionStateBoolean(thumbstickClickActionLeft);
                XrActionStateBoolean thumbstickClickStateRight = GetActionStateBoolean(thumbstickClickActionRight);

                // Update app logic based on input
                /* if (toggleState.changedSinceLastSync) {
                     // Also stop haptics
                     XrHapticActionInfo hapticActionInfo = {};
                     hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
                     hapticActionInfo.next = NULL;
                     hapticActionInfo.action = vibrateLeftFeedback;
                     OXR(xrStopHapticFeedback(appState.Session, &hapticActionInfo));
                     hapticActionInfo.action = vibrateRightFeedback;
                     OXR(xrStopHapticFeedback(appState.Session, &hapticActionInfo));
                 }*/

                /*if (thumbstickClickState.changedSinceLastSync &&
                    thumbstickClickState.currentState == XR_TRUE) {
                    float currentRefreshRate = 0.0f;
                    OXR(appState.pfnGetDisplayRefreshRate(appState.Session, &currentRefreshRate));
                    ALOGV("Current Display Refresh Rate: %f", currentRefreshRate);

                    const int requestedRateIndex = appState.RequestedDisplayRefreshRateIndex++ %
                        appState.NumSupportedDisplayRefreshRates;

                    const float requestRefreshRate =
                        appState.SupportedDisplayRefreshRates[requestedRateIndex];
                    ALOGV("Requesting Display Refresh Rate: %f", requestRefreshRate);
                    OXR(appState.pfnRequestDisplayRefreshRate(appState.Session, requestRefreshRate));
                }*/

                // The KHR simple profile doesn't have these actions, so the getters will fail
                // and flood the log with errors.
                // if (useSimpleProfile == false) {
                XrActionStateFloat moveXStateLeftSqueeze = GetActionStateFloat(moveOnXActionLeftSqueeze);
                XrActionStateFloat moveXStateRightSqueeze = GetActionStateFloat(moveOnXActionRightSqueeze);
                XrActionStateFloat moveYStateLeftTriggerValue = GetActionStateFloat(moveOnYActionLeftTriggerValue);
                XrActionStateFloat moveYStateRightTriggerValue = GetActionStateFloat(moveOnYActionRightTriggerValue);

                /* if (moveXStateLeftSqueeze.changedSinceLastSync) {
                     appQuadPositionX = moveXStateLeftSqueeze.currentState;
                 }
                 if (moveYStateLeftTriggerValue.changedSinceLastSync) {
                     appQuadPositionY = moveYStateLeftTriggerValue.currentState;
                 }*/

                XrActionStateVector2f moveJoystickStateLeft = GetActionStateVector2(moveOnJoystickActionLeft);
                XrActionStateVector2f moveJoystickStateRight = GetActionStateVector2(moveOnJoystickActionRight);
                /*if (moveJoystickState.changedSinceLastSync) {
                    appCylPositionX = moveJoystickState.currentState.x;
                    appCylPositionY = moveJoystickState.currentState.y;
                }*/
                //}

                // Haptics
                // NOTE: using the values from the example in the spec
                /* if (vibrateLeftState.changedSinceLastSync && vibrateLeftState.currentState) {
                     ALOGV("Firing Haptics on L ... ");
                     // fire haptics using output action
                     XrHapticVibration vibration = {};
                     vibration.type = XR_TYPE_HAPTIC_VIBRATION;
                     vibration.next = NULL;
                     vibration.amplitude = 0.5;
                     vibration.duration = ToXrTime(0.5); // half a second
                     vibration.frequency = 3000;
                     XrHapticActionInfo hapticActionInfo = {};
                     hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
                     hapticActionInfo.next = NULL;
                     hapticActionInfo.action = vibrateLeftFeedback;
                     OXR(xrApplyHapticFeedback(
                         appState.Session, &hapticActionInfo, (const XrHapticBaseHeader*)&vibration));
                 }
                 if (vibrateRightState.changedSinceLastSync && vibrateRightState.currentState) {
                     ALOGV("Firing Haptics on R ... ");
                     // fire haptics using output action
                     XrHapticVibration vibration = {};
                     vibration.type = XR_TYPE_HAPTIC_VIBRATION;
                     vibration.next = NULL;
                     vibration.amplitude = 0.5;
                     vibration.duration = XR_MIN_HAPTIC_DURATION;
                     vibration.frequency = 3000;
                     XrHapticActionInfo hapticActionInfo = {};
                     hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
                     hapticActionInfo.next = NULL;
                     hapticActionInfo.action = vibrateRightFeedback;
                     OXR(xrApplyHapticFeedback(
                         appState.Session, &hapticActionInfo, (const XrHapticBaseHeader*)&vibration));
                 }*/

                Env->CallVoidMethod(
                    app->activity->clazz, simulate_method, timeInSeconds, toggleStateLeftTrigger.changedSinceLastSync,
                    toggleStateLeftTrigger.currentState, toggleStateRightTrigger.changedSinceLastSync,
                    toggleStateRightTrigger.currentState, toggleStateLeftSqueeze.changedSinceLastSync,
                    toggleStateLeftSqueeze.currentState, toggleStateRightSqueeze.changedSinceLastSync,
                    toggleStateRightSqueeze.currentState, toggleStateLeftX.changedSinceLastSync,
                    toggleStateLeftX.currentState, toggleStateLeftY.changedSinceLastSync, toggleStateLeftY.currentState,
                    toggleStateMenu.changedSinceLastSync, toggleStateMenu.currentState,
                    toggleStateRightA.changedSinceLastSync, toggleStateRightA.currentState,
                    toggleStateRightB.changedSinceLastSync, toggleStateRightB.currentState,
                    thumbstickClickStateLeft.changedSinceLastSync, thumbstickClickStateLeft.currentState,
                    thumbstickClickStateRight.changedSinceLastSync, thumbstickClickStateRight.currentState,
                    moveXStateLeftSqueeze.changedSinceLastSync, moveXStateLeftSqueeze.currentState,
                    moveXStateRightSqueeze.changedSinceLastSync, moveXStateRightSqueeze.currentState,
                    moveYStateLeftTriggerValue.changedSinceLastSync, moveYStateLeftTriggerValue.currentState,
                    moveYStateRightTriggerValue.changedSinceLastSync, moveYStateRightTriggerValue.currentState,
                    moveJoystickStateLeft.changedSinceLastSync, moveJoystickStateLeft.currentState.x,
                    moveJoystickStateLeft.currentState.y, moveJoystickStateRight.changedSinceLastSync,
                    moveJoystickStateRight.currentState.x, moveJoystickStateRight.currentState.y,
                    appState.Scene.TrackedController[0].Active, appState.Scene.TrackedController[0].Pose.position.x,
                    appState.Scene.TrackedController[0].Pose.position.y,
                    appState.Scene.TrackedController[0].Pose.position.z,
                    appState.Scene.TrackedController[0].Pose.orientation.w,
                    appState.Scene.TrackedController[0].Pose.orientation.x,
                    appState.Scene.TrackedController[0].Pose.orientation.y,
                    appState.Scene.TrackedController[0].Pose.orientation.z, appState.Scene.TrackedController[1].Active,
                    appState.Scene.TrackedController[1].Pose.position.x,
                    appState.Scene.TrackedController[1].Pose.position.y,
                    appState.Scene.TrackedController[1].Pose.position.z,
                    appState.Scene.TrackedController[1].Pose.orientation.w,
                    appState.Scene.TrackedController[1].Pose.orientation.x,
                    appState.Scene.TrackedController[1].Pose.orientation.y,
                    appState.Scene.TrackedController[1].Pose.orientation.z, appState.Scene.TrackedController[2].Active,
                    appState.Scene.TrackedController[2].Pose.position.x,
                    appState.Scene.TrackedController[2].Pose.position.y,
                    appState.Scene.TrackedController[2].Pose.position.z,
                    appState.Scene.TrackedController[2].Pose.orientation.w,
                    appState.Scene.TrackedController[2].Pose.orientation.x,
                    appState.Scene.TrackedController[2].Pose.orientation.y,
                    appState.Scene.TrackedController[2].Pose.orientation.z, appState.Scene.TrackedController[3].Active,
                    appState.Scene.TrackedController[3].Pose.position.x,
                    appState.Scene.TrackedController[3].Pose.position.y,
                    appState.Scene.TrackedController[3].Pose.position.z,
                    appState.Scene.TrackedController[3].Pose.orientation.w,
                    appState.Scene.TrackedController[3].Pose.orientation.x,
                    appState.Scene.TrackedController[3].Pose.orientation.y,
                    appState.Scene.TrackedController[3].Pose.orientation.z);
            }

            // Set-up the compositor layers for this frame.
            // NOTE: Multiple independent layers are allowed, but they need to be added
            // in a depth consistent order.

            XrCompositionLayerProjectionView projection_layer_elements[2] = {};

            appState.LayerCount = 0;
            memset(appState.Layers, 0, sizeof(ovrCompositorLayer_Union) * ovrMaxLayerCount);
            bool shouldRenderWorldLayer = true;
            bool hasCubeMapBackground = appState.Scene.CubeMapSwapChain.Handle != XR_NULL_HANDLE;

            // Add a background Layer
            /*if (appState.Scene.BackGroundType == BACKGROUND_CUBEMAP &&
                hasCubeMapBackground ) {
                XrCompositionLayerCubeKHR cube_layer = {};
                cube_layer.type = XR_TYPE_COMPOSITION_LAYER_CUBE_KHR;
                cube_layer.layerFlags = 0;
                cube_layer.space = appState.CurrentSpace;
                cube_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                cube_layer.swapchain = appState.Scene.CubeMapSwapChain.Handle;
                cube_layer.orientation = XrQuaternionf_Identity();

                appState.Layers[appState.LayerCount++].Cube = cube_layer;
                shouldRenderWorldLayer = false;
            } else if (appState.Scene.BackGroundType == BACKGROUND_EQUIRECT) {
                XrCompositionLayerEquirect2KHR equirect_layer = {};
                equirect_layer.type = XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR;
                equirect_layer.layerFlags = 0;
                equirect_layer.space = appState.CurrentSpace;
                equirect_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                memset(&equirect_layer.subImage, 0, sizeof(XrSwapchainSubImage));
                equirect_layer.subImage.swapchain = appState.Scene.EquirectSwapChain.Handle;
                equirect_layer.subImage.imageRect.offset.x = 0;
                equirect_layer.subImage.imageRect.offset.y = 0;
                equirect_layer.subImage.imageRect.extent.width = appState.Scene.EquirectSwapChain.Width;
                equirect_layer.subImage.imageRect.extent.height =
                    appState.Scene.EquirectSwapChain.Height;
                equirect_layer.subImage.imageArrayIndex = 0;
                equirect_layer.pose = XrPosef_Identity();
                equirect_layer.radius = 10.0f;
                const float centralHorizontalAngle = (2.0f * MATH_PI) / 3.0f;
                const float upperVerticalAngle =
                    (MATH_PI / 2.0f) * (2.0f / 3.0f); // 60 degrees north latitude
                const float lowerVerticalAngle = 0.0f; // equator
                equirect_layer.centralHorizontalAngle = centralHorizontalAngle;
                equirect_layer.upperVerticalAngle = upperVerticalAngle;
                equirect_layer.lowerVerticalAngle = lowerVerticalAngle;

                appState.Layers[appState.LayerCount++].Equirect2 = equirect_layer;
            }*/

            // Render the world-view layer (simple ground plane)
            if (shouldRenderWorldLayer) {

                ovrRenderer_RenderFrame(Env, app->activity->clazz, &appState.Renderer, &appState.Scene, &sceneMatrices);

                XrCompositionLayerProjection projection_layer = {};
                projection_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
                projection_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                projection_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
                projection_layer.space = appState.CurrentSpace;
                projection_layer.viewCount = ovrMaxNumEyes;
                projection_layer.views = projection_layer_elements;

                for (int eye = 0; eye < ovrMaxNumEyes; eye++) {
                    ovrFramebuffer *frameBuffer = &appState.Renderer.FrameBuffer[eye];

                    memset(&projection_layer_elements[eye], 0, sizeof(XrCompositionLayerProjectionView));
                    projection_layer_elements[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;

                    projection_layer_elements[eye].pose = XrPosef_Inverse(viewTransform[eye]);
                    projection_layer_elements[eye].fov = projections[eye].fov;

                    memset(&projection_layer_elements[eye].subImage, 0, sizeof(XrSwapchainSubImage));
                    projection_layer_elements[eye].subImage.swapchain = frameBuffer->ColorSwapChain.Handle;
                    projection_layer_elements[eye].subImage.imageRect.offset.x = 0;
                    projection_layer_elements[eye].subImage.imageRect.offset.y = 0;
                    projection_layer_elements[eye].subImage.imageRect.extent.width = frameBuffer->ColorSwapChain.Width;
                    projection_layer_elements[eye].subImage.imageRect.extent.height =
                        frameBuffer->ColorSwapChain.Height;
                    projection_layer_elements[eye].subImage.imageArrayIndex = 0;
                }

                appState.Layers[appState.LayerCount++].Projection = projection_layer;
            }

            // Build the cylinder layer
            /*{
                XrCompositionLayerCylinderKHR cylinder_layer = {};
                cylinder_layer.type = XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR;
                cylinder_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                cylinder_layer.space = appState.LocalSpace;
                cylinder_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
                memset(&cylinder_layer.subImage, 0, sizeof(XrSwapchainSubImage));
                cylinder_layer.subImage.swapchain = appState.Scene.CylinderSwapChain.Handle;
                cylinder_layer.subImage.imageRect.offset.x = 0;
                cylinder_layer.subImage.imageRect.offset.y = 0;
                cylinder_layer.subImage.imageRect.extent.width = appState.Scene.CylinderSwapChain.Width;
                cylinder_layer.subImage.imageRect.extent.height =
                    appState.Scene.CylinderSwapChain.Height;
                cylinder_layer.subImage.imageArrayIndex = 0;
                const XrVector3f axis = {0.0f, 1.0f, 0.0f};
                const XrVector3f pos = {appCylPositionX, appCylPositionY, 0.0f};
                cylinder_layer.pose.orientation =
                    XrQuaternionf_CreateFromVectorAngle(axis, -45.0f * MATH_PI / 180.0f);
                cylinder_layer.pose.position = pos;
                cylinder_layer.radius = 2.0f;

                cylinder_layer.centralAngle = MATH_PI / 4.0;
                cylinder_layer.aspectRatio = 2.0f;

                appState.Layers[appState.LayerCount++].Cylinder = cylinder_layer;
            }*/

            // Build the quad layer
            /*{
                const XrVector3f axis = {0.0f, 1.0f, 0.0f};
                XrVector3f pos = {
                    -2.0f * (1.0f - appQuadPositionX), 2.0f * (1.0f - appQuadPositionY), -2.0f};
                XrExtent2Df size = {1.0f, 1.0f};

                XrCompositionLayerQuad quad_layer_left = {};
                quad_layer_left.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
                quad_layer_left.next = NULL;
                quad_layer_left.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                quad_layer_left.space = appState.CurrentSpace;
                quad_layer_left.eyeVisibility = XR_EYE_VISIBILITY_LEFT;
                memset(&quad_layer_left.subImage, 0, sizeof(XrSwapchainSubImage));
                quad_layer_left.subImage.swapchain = appState.Scene.QuadSwapChain.Handle;
                quad_layer_left.subImage.imageRect.offset.x = 0;
                quad_layer_left.subImage.imageRect.offset.y = 0;
                quad_layer_left.subImage.imageRect.extent.width = appState.Scene.QuadSwapChain.Width;
                quad_layer_left.subImage.imageRect.extent.height = appState.Scene.QuadSwapChain.Height;
                quad_layer_left.subImage.imageArrayIndex = 0;
                quad_layer_left.pose = XrPosef_Identity();
                quad_layer_left.pose.orientation =
                    XrQuaternionf_CreateFromVectorAngle(axis, 45.0f * MATH_PI / 180.0f);
                quad_layer_left.pose.position = pos;
                quad_layer_left.size = size;

                appState.Layers[appState.LayerCount++].Quad = quad_layer_left;

                XrCompositionLayerQuad quad_layer_right = {};
                quad_layer_right.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
                quad_layer_right.next = NULL;
                quad_layer_right.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                quad_layer_right.space = appState.CurrentSpace;
                quad_layer_right.eyeVisibility = XR_EYE_VISIBILITY_RIGHT;
                memset(&quad_layer_right.subImage, 0, sizeof(XrSwapchainSubImage));
                quad_layer_right.subImage.swapchain = appState.Scene.QuadSwapChain.Handle;
                quad_layer_right.subImage.imageRect.offset.x = 0;
                quad_layer_right.subImage.imageRect.offset.y = 0;
                quad_layer_right.subImage.imageRect.extent.width = appState.Scene.QuadSwapChain.Width;
                quad_layer_right.subImage.imageRect.extent.height = appState.Scene.QuadSwapChain.Height;
                quad_layer_right.subImage.imageArrayIndex = 0;
                quad_layer_right.pose = XrPosef_Identity();
                quad_layer_right.pose.orientation =
                    XrQuaternionf_CreateFromVectorAngle(axis, 45.0f * MATH_PI / 180.0f);
                quad_layer_right.pose.position = pos;
                quad_layer_right.size = size;

                appState.Layers[appState.LayerCount++].Quad = quad_layer_right;
            }*/

            // Compose the layers for this frame.
            const XrCompositionLayerBaseHeader *layers[ovrMaxLayerCount] = {};
            for (int i = 0; i < appState.LayerCount; i++) {
                layers[i] = (const XrCompositionLayerBaseHeader *)&appState.Layers[i];
            }

            XrFrameEndInfo endFrameInfo = {};
            endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
            endFrameInfo.displayTime = frameState.predictedDisplayTime;
            endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            endFrameInfo.layerCount = appState.LayerCount;
            endFrameInfo.layers = layers;

            OXR(xrEndFrame(appState.Session, &endFrameInfo));
        }
        app->activity->vm->DetachCurrentThread();
    }

    XrAction GameLoop::CreateAction(
        XrActionSet actionSet, XrActionType type, const char *actionName, const char *localizedName,
        int countSubactionPaths, XrPath *subactionPaths) {
        ALOGV("CreateAction %s, %" PRIi32, actionName, countSubactionPaths);

        XrActionCreateInfo aci = {};
        aci.type = XR_TYPE_ACTION_CREATE_INFO;
        aci.next = NULL;
        aci.actionType = type;
        if (countSubactionPaths > 0) {
            aci.countSubactionPaths = countSubactionPaths;
            aci.subactionPaths = subactionPaths;
        }
        strcpy(aci.actionName, actionName);
        strcpy(aci.localizedActionName, localizedName ? localizedName : actionName);
        XrAction action = XR_NULL_HANDLE;
        OXR(xrCreateAction(actionSet, &aci, &action));
        return action;
    }

    XrActionSuggestedBinding GameLoop::ActionSuggestedBinding(XrAction action, const char *bindingString) {
        XrActionSuggestedBinding asb;
        asb.action = action;
        XrPath bindingPath;
        OXR(xrStringToPath(appState.Instance, bindingString, &bindingPath));
        asb.binding = bindingPath;
        return asb;
    }

    XrSpace GameLoop::CreateActionSpace(XrAction poseAction, XrPath subactionPath) {
        XrActionSpaceCreateInfo asci = {};
        asci.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        asci.action = poseAction;
        asci.poseInActionSpace.orientation.w = 1.0f;
        asci.subactionPath = subactionPath;
        XrSpace actionSpace = XR_NULL_HANDLE;
        OXR(xrCreateActionSpace(appState.Session, &asci, &actionSpace));
        return actionSpace;
    }

    XrActionStateBoolean GameLoop::GetActionStateBoolean(XrAction action) {
        XrActionStateGetInfo getInfo = {};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = action;

        XrActionStateBoolean state = {};
        state.type = XR_TYPE_ACTION_STATE_BOOLEAN;

        OXR(xrGetActionStateBoolean(appState.Session, &getInfo, &state));
        return state;
    }

    XrActionStateFloat GameLoop::GetActionStateFloat(XrAction action) {
        XrActionStateGetInfo getInfo = {};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = action;

        XrActionStateFloat state = {};
        state.type = XR_TYPE_ACTION_STATE_FLOAT;

        OXR(xrGetActionStateFloat(appState.Session, &getInfo, &state));
        return state;
    }

    XrActionStateVector2f GameLoop::GetActionStateVector2(XrAction action) {
        XrActionStateGetInfo getInfo = {};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = action;

        XrActionStateVector2f state = {};
        state.type = XR_TYPE_ACTION_STATE_VECTOR2F;

        OXR(xrGetActionStateVector2f(appState.Session, &getInfo, &state));
        return state;
    }

    XrActionSet GameLoop::CreateActionSet(int priority, const char *name, const char *localizedName) {
        XrActionSetCreateInfo asci = {};
        asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        asci.next = NULL;
        asci.priority = priority;
        strcpy(asci.actionSetName, name);
        strcpy(asci.localizedActionSetName, localizedName);
        XrActionSet actionSet = XR_NULL_HANDLE;
        OXR(xrCreateActionSet(appState.Instance, &asci, &actionSet));
        return actionSet;
    }

    void GameLoop::UpdateStageBounds(ovrApp *pappState) {
        XrExtent2Df stageBounds = {};

        XrResult result;
        OXR(result = xrGetReferenceSpaceBoundsRect(pappState->Session, XR_REFERENCE_SPACE_TYPE_STAGE, &stageBounds));
        if (result != XR_SUCCESS) {
            ALOGV("Stage bounds query failed: using small defaults");
            stageBounds.width = 1.0f;
            stageBounds.height = 1.0f;

            pappState->CurrentSpace = pappState->FakeStageSpace;
        }

        ALOGV("Stage bounds: width = %f, depth %f", stageBounds.width, stageBounds.height);

        const float halfWidth = stageBounds.width * 0.5f;
        const float halfDepth = stageBounds.height * 0.5f;

        ovrGeometry_Destroy(&pappState->Scene.GroundPlane);
        ovrGeometry_DestroyVAO(&pappState->Scene.GroundPlane);
        ovrGeometry_CreateStagePlane(&pappState->Scene.GroundPlane, -halfWidth, -halfDepth, halfWidth, halfDepth);
        ovrGeometry_CreateVAO(&pappState->Scene.GroundPlane);
    }

    bool GameLoop::ActionPoseIsActive(XrAction action, XrPath subactionPath) {
        XrActionStateGetInfo getInfo = {};
        getInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
        getInfo.action = action;
        getInfo.subactionPath = subactionPath;

        XrActionStatePose state = {};
        state.type = XR_TYPE_ACTION_STATE_POSE;
        OXR(xrGetActionStatePose(appState.Session, &getInfo, &state));
        return state.isActive != XR_FALSE;
    }

    LocVel GameLoop::GetSpaceLocVel(XrSpace space, XrTime time) {
        LocVel lv = {{}};
        lv.loc.type = XR_TYPE_SPACE_LOCATION;
        lv.loc.next = &lv.vel;
        lv.vel.type = XR_TYPE_SPACE_VELOCITY;
        OXR(xrLocateSpace(space, appState.CurrentSpace, time, &lv.loc));
        lv.loc.next = NULL; // pointer no longer valid or necessary
        return lv;
    }
}