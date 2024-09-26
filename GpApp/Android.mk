LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := GpApp

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../PortabilityLayer

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0 -DGP_ZLIB_BUILTIN=1

# Add your application source files here...
LOCAL_SRC_FILES := \
	GpApp_Combined.cpp

LOCAL_STATIC_LIBRARIES := PortabilityLayer

include $(BUILD_SHARED_LIBRARY)
