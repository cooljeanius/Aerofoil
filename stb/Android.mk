LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := stb

# Add your application source files here...
LOCAL_SRC_FILES := \
	stb_image_write.c
	

include $(BUILD_STATIC_LIBRARY)
