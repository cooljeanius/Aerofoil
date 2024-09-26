LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := PortabilityLayer

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../rapidjson/include	\
	$(LOCAL_PATH)/../MacRomanConversion	\
	$(LOCAL_PATH)/../stb

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0 -DGP_ZLIB_BUILTIN=1

LOCAL_EXPORT_LDLIBS := -lz

# Add your application source files here...
LOCAL_SRC_FILES := \
	PortabilityLayer_Combined.cpp
	

LOCAL_STATIC_LIBRARIES := MacRomanConversion stb

include $(BUILD_STATIC_LIBRARY)
