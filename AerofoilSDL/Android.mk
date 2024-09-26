LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := AerofoilSDL

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../GpCommon	\
	$(LOCAL_PATH)/../GpShell	\
	$(LOCAL_PATH)/../Common	\
	$(LOCAL_PATH)/../PortabilityLayer	\
	$(LOCAL_PATH)/../AerofoilPortable	\
	$(LOCAL_PATH)/$(SDL_PATH)/include

LOCAL_CFLAGS := -DGP_DEBUG_CONFIG=0

# Add your application source files here...
LOCAL_SRC_FILES := \
	GpAudioDriver_SDL2.cpp	\
	GpDisplayDriver_SDL_GL2.cpp	\
	GpInputDriver_SDL_Gamepad.cpp	\
	ShaderCode/CopyQuadP.cpp	\
	ShaderCode/DrawQuadPaletteP.cpp	\
	ShaderCode/DrawQuad32P.cpp	\
	ShaderCode/DrawQuadV.cpp	\
	ShaderCode/ScaleQuadP.cpp

include $(BUILD_STATIC_LIBRARY)
