LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := GpShell

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../Common \
	$(LOCAL_PATH)/../GpCommon \
	$(LOCAL_PATH)/../PortabilityLayer

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0

# Add your application source files here...
LOCAL_SRC_FILES := \
	GpShell_Combined.cpp

include $(BUILD_STATIC_LIBRARY)
