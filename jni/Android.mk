LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := xrcompositor


LOCAL_CPPFLAGS += -std=c++17 -Werror
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../j4q/3rdParty/khronos/openxr/OpenXR-SDK/include/ $(LOCAL_PATH)//src

#traverse all the directory and subdirectory
define walk
  $(wildcard $(1)) $(foreach e, $(wildcard $(1)/*), $(call walk, $(e)))
endef

#find all the file recursively under jni/
ALLFILES = $(call walk, $(LOCAL_PATH))
FILE_LIST := $(filter %.cpp, $(ALLFILES))


LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

#LOCAL_SRC_FILES := XrCompositor_NativeActivity.cpp

LOCAL_LDLIBS := -lEGL -lGLESv3 -landroid -llog

LOCAL_LDFLAGS := -u ANativeActivity_onCreate

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := openxr_loader

include $(BUILD_SHARED_LIBRARY)

$(call import-module,OpenXR/Projects/AndroidPrebuilt/jni)
$(call import-module,android/native_app_glue)
