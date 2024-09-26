LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := AerofoilPortable

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../GpShell	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../PortabilityLayer

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0

# Add your application source files here...
LOCAL_SRC_FILES := \
	GpAllocator_C.cpp	\
	GpThreadEvent_Cpp11.cpp	\
	GpSystemServices_POSIX.cpp


include $(BUILD_STATIC_LIBRARY)
