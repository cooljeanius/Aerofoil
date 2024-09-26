LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := MacRomanConversion

# Add your application source files here...
LOCAL_SRC_FILES := \
	MacRomanConversion.cpp
	

include $(BUILD_STATIC_LIBRARY)
