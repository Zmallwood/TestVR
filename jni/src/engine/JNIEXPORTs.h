#pragma once
#include <android/native_window_jni.h>
#include "Global.h"
#define DEBUG 1
#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr_platform.h>

namespace nar
{
    inline static XrAction vibrateLeftFeedback;
    inline static XrAction vibrateRightFeedback;

    JNIEXPORT static jlong JNICALL Java_edu_ufl_digitalworlds_j4q_J4Q_stopHapticFeedbackLeft(JNIEnv *env, jclass obj) {
        XrHapticActionInfo hapticActionInfo = {};
        hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
        hapticActionInfo.next = NULL;
        hapticActionInfo.action = vibrateLeftFeedback;
        OXR(xrStopHapticFeedback(appState.Session, &hapticActionInfo));
        return (jlong)0;
    }

    JNIEXPORT static jlong JNICALL Java_edu_ufl_digitalworlds_j4q_J4Q_stopHapticFeedbackRight(JNIEnv *env, jclass obj) {
        XrHapticActionInfo hapticActionInfo = {};
        hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
        hapticActionInfo.next = NULL;
        hapticActionInfo.action = vibrateRightFeedback;
        OXR(xrStopHapticFeedback(appState.Session, &hapticActionInfo));
        return (jlong)0;
    }

    JNIEXPORT static jlong JNICALL Java_edu_ufl_digitalworlds_j4q_J4Q_applyHapticFeedbackLeft(
        JNIEnv *env, jclass obj, jfloat amplitude, jfloat seconds, jint frequency) {
        XrHapticVibration vibration = {};
        vibration.type = XR_TYPE_HAPTIC_VIBRATION;
        vibration.next = NULL;
        vibration.amplitude = amplitude;
        vibration.duration = ToXrTime(seconds); // half a second
        vibration.frequency = frequency;
        XrHapticActionInfo hapticActionInfo = {};
        hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
        hapticActionInfo.next = NULL;
        hapticActionInfo.action = vibrateLeftFeedback;
        OXR(xrApplyHapticFeedback(appState.Session, &hapticActionInfo, (const XrHapticBaseHeader *)&vibration));
        return (jlong)0;
    }

    JNIEXPORT static jlong JNICALL Java_edu_ufl_digitalworlds_j4q_J4Q_applyHapticFeedbackRight(
        JNIEnv *env, jclass obj, jfloat amplitude, jfloat seconds, jint frequency) {
        XrHapticVibration vibration = {};
        vibration.type = XR_TYPE_HAPTIC_VIBRATION;
        vibration.next = NULL;
        vibration.amplitude = amplitude;
        vibration.duration = ToXrTime(seconds); // half a second
        vibration.frequency = frequency;
        XrHapticActionInfo hapticActionInfo = {};
        hapticActionInfo.type = XR_TYPE_HAPTIC_ACTION_INFO;
        hapticActionInfo.next = NULL;
        hapticActionInfo.action = vibrateRightFeedback;
        OXR(xrApplyHapticFeedback(appState.Session, &hapticActionInfo, (const XrHapticBaseHeader *)&vibration));
        return (jlong)0;
    }
}