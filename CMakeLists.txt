cmake_minimum_required(VERSION 3.10)
project (Aerofoil)
SET(PLATFORM "X" CACHE STRING "Defines the target platform")
SET(EXECNAME "AerofoilX" CACHE STRING "Defines the exec name")

message(${CMAKE_BINARY_DIR})

# FIXME: Clang treats this as an error by default; fixing the source rather
# than downgrading the error to a warning would be a better solution.
add_compile_options(
	$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-error=c++11-narrowing>
	)

find_package(SDL2 REQUIRED)

if(PLATFORM STREQUAL "MAC")
	SET(EXECNAME "AerofoilMac" CACHE STRING "Defines the exec name" FORCE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_definitions(-D__MACOS__)
endif()

message("Building ${EXECNAME} for: ${PLATFORM}")

add_definitions(-DGP_DEBUG_CONFIG=0)
add_definitions(-DNDEBUG=1)


add_library(stb STATIC
	stb/stb_image_write.c
	)

add_library(zlib STATIC
	zlib/adler32.c
	zlib/crc32.c
	zlib/deflate.c
	zlib/inffast.c
	zlib/inflate.c
	zlib/inftrees.c
	zlib/trees.c
	zlib/zutil.c
	)

add_library(MacRomanConversion STATIC
	MacRomanConversion/MacRomanConversion.cpp
	)

add_library(PortabilityLayer STATIC
	PortabilityLayer/AntiAliasTable.cpp
	PortabilityLayer/AppEventHandler.cpp
	PortabilityLayer/BinHex4.cpp
	PortabilityLayer/BitmapImage.cpp
	PortabilityLayer/ByteSwap.cpp
	PortabilityLayer/CFileStream.cpp
	PortabilityLayer/CompositeRenderedFont.cpp
	PortabilityLayer/DeflateCodec.cpp
	PortabilityLayer/DialogManager.cpp
	PortabilityLayer/DisplayDeviceManager.cpp
	PortabilityLayer/EllipsePlotter.cpp
	PortabilityLayer/FileBrowserUI.cpp
	PortabilityLayer/FileManager.cpp
	PortabilityLayer/FileSectionStream.cpp
	PortabilityLayer/FontFamily.cpp
	PortabilityLayer/FontManager.cpp
	PortabilityLayer/FontRenderer.cpp
	PortabilityLayer/GPArchive.cpp
	PortabilityLayer/HostSuspendHook.cpp
	PortabilityLayer/IconLoader.cpp
	PortabilityLayer/InflateStream.cpp
	PortabilityLayer/InputManager.cpp
	PortabilityLayer/LinePlotter.cpp
	PortabilityLayer/MacBinary2.cpp
	PortabilityLayer/MacFileInfo.cpp
	PortabilityLayer/MacFileMem.cpp
	PortabilityLayer/MemoryManager.cpp
	PortabilityLayer/MemReaderStream.cpp
	PortabilityLayer/MenuManager.cpp
	PortabilityLayer/MMHandleBlock.cpp
	PortabilityLayer/PLApplication.cpp
	PortabilityLayer/PLButtonWidget.cpp
	PortabilityLayer/PLControlDefinitions.cpp
	PortabilityLayer/PLCore.cpp
	PortabilityLayer/PLCTabReducer.cpp
	PortabilityLayer/PLDialogs.cpp
	PortabilityLayer/PLDrivers.cpp
	PortabilityLayer/PLEditboxWidget.cpp
	PortabilityLayer/PLEventQueue.cpp
	PortabilityLayer/PLHacks.cpp
	PortabilityLayer/PLHandle.cpp
	PortabilityLayer/PLIconWidget.cpp
	PortabilityLayer/PLImageWidget.cpp
	PortabilityLayer/PLInvisibleWidget.cpp
	PortabilityLayer/PLKeyEncoding.cpp
	PortabilityLayer/PLLabelWidget.cpp
	PortabilityLayer/PLMenus.cpp
	PortabilityLayer/PLMovies.cpp
	PortabilityLayer/PLNumberFormatting.cpp
	PortabilityLayer/PLPopupMenuWidget.cpp
	PortabilityLayer/PLQDOffscreen.cpp
	PortabilityLayer/PLQDraw.cpp
	PortabilityLayer/PLResourceManager.cpp
	PortabilityLayer/PLResources.cpp
	PortabilityLayer/PLScrollBarWidget.cpp
	PortabilityLayer/PLSound.cpp
	PortabilityLayer/PLStandardColors.cpp
	PortabilityLayer/PLStringCompare.cpp
	PortabilityLayer/PLSysCalls.cpp
	PortabilityLayer/PLTimeTaggedVOSEvent.cpp
	PortabilityLayer/PLWidgets.cpp
	PortabilityLayer/QDGraf.cpp
	PortabilityLayer/QDManager.cpp
	PortabilityLayer/QDPictDecoder.cpp
	PortabilityLayer/QDPictEmitContext.cpp
	PortabilityLayer/QDPictHeader.cpp
	PortabilityLayer/QDPixMap.cpp
	PortabilityLayer/QDPort.cpp
	PortabilityLayer/QDStandardPalette.cpp
	PortabilityLayer/RandomNumberGenerator.cpp
	PortabilityLayer/ResolveCachingColor.cpp
	PortabilityLayer/ResourceCompiledRef.cpp
	PortabilityLayer/ResourceFile.cpp
	PortabilityLayer/ScanlineMask.cpp
	PortabilityLayer/ScanlineMaskBuilder.cpp
	PortabilityLayer/ScanlineMaskConverter.cpp
	PortabilityLayer/ScanlineMaskIterator.cpp
	PortabilityLayer/SimpleGraphic.cpp
	PortabilityLayer/TextPlacer.cpp
	PortabilityLayer/UTF8.cpp
	PortabilityLayer/WindowDef.cpp
	PortabilityLayer/WindowManager.cpp
	PortabilityLayer/WorkerThread.cpp
	PortabilityLayer/XModemCRC.cpp
	PortabilityLayer/ZipFileProxy.cpp
	)

target_include_directories(PortabilityLayer PRIVATE
	Common
	GpCommon
	PortabilityLayer
	zlib
	rapidjson/include
	MacRomanConversion
	stb
	)

target_compile_options(PortabilityLayer PRIVATE -Wno-multichar)

target_link_libraries(PortabilityLayer PRIVATE zlib MacRomanConversion stb)


add_library(GpShell STATIC
	GpShell/GpAppEnvironment.cpp
	GpShell/GpAudioDriverFactory.cpp
	GpShell/GpDisplayDriverFactory.cpp
	GpShell/GpFontHandlerFactory.cpp
	GpShell/GpGlobalConfig.cpp
	GpShell/GpInputDriverFactory.cpp
	GpShell/GpMain.cpp
	GpShell/GpVOSEventQueue.cpp
	)

target_include_directories(GpShell PRIVATE
	Common
	GpCommon
	PortabilityLayer
	)

add_library(GpApp STATIC
	GpApp/About.cpp
	GpApp/AnimCursor.cpp
	GpApp/AppleEvents.cpp
	GpApp/Banner.cpp
	GpApp/ColorUtils.cpp
	GpApp/Coordinates.cpp
	GpApp/DialogUtils.cpp
	GpApp/DynamicMaps.cpp
	GpApp/Dynamics.cpp
	GpApp/Dynamics2.cpp
	GpApp/Dynamics3.cpp
	GpApp/Environ.cpp
	GpApp/Events.cpp
	GpApp/FileError.cpp
	GpApp/GameOver.cpp
	GpApp/GpAppInterface.cpp
	GpApp/Grease.cpp
	GpApp/HighScores.cpp
	GpApp/House.cpp
	GpApp/HouseInfo.cpp
	GpApp/HouseIO.cpp
	GpApp/HouseLegal.cpp
	GpApp/Input.cpp
	GpApp/Interactions.cpp
	GpApp/InterfaceInit.cpp
	GpApp/Link.cpp
	GpApp/Main.cpp
	GpApp/MainMenuUI.cpp
	GpApp/MainWindow.cpp
	GpApp/Map.cpp
	GpApp/Marquee.cpp
	GpApp/Menu.cpp
	GpApp/Modes.cpp
	GpApp/Music.cpp
	GpApp/ObjectAdd.cpp
	GpApp/ObjectDraw.cpp
	GpApp/ObjectDraw2.cpp
	GpApp/ObjectDrawAll.cpp
	GpApp/ObjectEdit.cpp
	GpApp/ObjectInfo.cpp
	GpApp/ObjectRects.cpp
	GpApp/Objects.cpp
	GpApp/Play.cpp
	GpApp/Player.cpp
	GpApp/Prefs.cpp
	GpApp/RectUtils.cpp
	GpApp/Render.cpp
	GpApp/Room.cpp
	GpApp/RoomGraphics.cpp
	GpApp/RoomInfo.cpp
	GpApp/RubberBands.cpp
	GpApp/SavedGames.cpp
	GpApp/Scoreboard.cpp
	GpApp/Scrap.cpp
	GpApp/SelectHouse.cpp
	GpApp/Settings.cpp
	GpApp/Sound.cpp
	GpApp/SoundSync_Cpp11.cpp
	GpApp/SourceExport.cpp
	GpApp/StringUtils.cpp
	GpApp/StructuresInit.cpp
	GpApp/StructuresInit2.cpp
	GpApp/Tools.cpp
	GpApp/Transit.cpp
	GpApp/Transitions.cpp
	GpApp/Triggers.cpp
	GpApp/Trip.cpp
	GpApp/Utilities.cpp
	GpApp/WindowUtils.cpp
	)

target_compile_options(GpApp PRIVATE -Wno-multichar)

target_include_directories(GpApp PRIVATE
	Common
	GpCommon
	PortabilityLayer
	)

target_link_libraries(GpApp PRIVATE PortabilityLayer)

if(CMAKE_HOST_UNIX)
	set(EXEC_SOURCES
		AerofoilPortable/GpSystemServices_POSIX.cpp
		AerofoilPortable/GpThreadEvent_Cpp11.cpp
		AerofoilPortable/GpAllocator_C.cpp
		AerofoilSDL/GpAudioDriver_SDL2.cpp
		AerofoilSDL/GpDisplayDriver_SDL_GL2.cpp
		AerofoilSDL/GpInputDriver_SDL_Gamepad.cpp
		AerofoilSDL/ShaderCode/CopyQuadP.cpp
		AerofoilSDL/ShaderCode/DrawQuad32P.cpp
		AerofoilSDL/ShaderCode/DrawQuadPaletteP.cpp
		AerofoilSDL/ShaderCode/DrawQuadV.cpp
		AerofoilSDL/ShaderCode/ScaleQuadP.cpp
		AerofoilX/GpMain_SDL_X.cpp
		AerofoilX/GpLogDriver_X.cpp
		AerofoilX/GpSystemServices_X.cpp
		AerofoilX/GpFileSystem_X.cpp
		)

	set(EXEC_LIBS
		${SDL2_LIBRARIES}
		GpApp
		GpShell
		PortabilityLayer
		)

	set(EXEC_INC_DIRS
		Common
		GpCommon
		GpShell
		AerofoilSDL
		AerofoilPortable
		PortabilityLayer
		${SDL2_INCLUDE_DIRS}
		)
	if(PLATFORM STREQUAL "MAC")
		list(APPEND EXEC_SOURCES
			AerofoilMac/AerofoilMac/AerofoilApplication.mm
			AerofoilMac/AerofoilMac/MacInit.mm
			)
		list(APPEND EXEC_INC_DIRS
			AerofoilMac/AerofoilMac
			)
		list(APPEND EXEC_LIBS
			"-framework Cocoa"
			)
	endif(PLATFORM STREQUAL "MAC")

	add_executable(${EXECNAME} ${EXEC_SOURCES})
	target_include_directories(${EXECNAME} PRIVATE ${EXEC_INC_DIRS})
	target_link_libraries(${EXECNAME} ${EXEC_LIBS})
endif()


add_executable(flattenmov flattenmov/flattenmov.cpp AerofoilPortable/GpAllocator_C.cpp)
target_include_directories(flattenmov PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	)
target_link_libraries(flattenmov PortabilityLayer)

add_executable(bin2gp bin2gp/bin2gp.cpp AerofoilPortable/GpAllocator_C.cpp)
target_include_directories(bin2gp PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	)
target_link_libraries(bin2gp PortabilityLayer)

add_executable(hqx2bin hqx2bin/hqx2bin.cpp AerofoilPortable/GpAllocator_C.cpp)
target_include_directories(hqx2bin PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	)
target_link_libraries(hqx2bin PortabilityLayer)

add_executable(hqx2gp
	hqx2gp/hqx2gp.cpp
	AerofoilPortable/GpAllocator_C.cpp
	WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp
	)
target_include_directories(hqx2gp PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	WindowsUnicodeToolShim
	)
target_link_libraries(hqx2gp PortabilityLayer)

add_executable(gpr2gpa
	gpr2gpa/gpr2gpa.cpp
	gpr2gpa/macedec.cpp
	AerofoilPortable/GpAllocator_C.cpp
	WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp
	)
target_include_directories(gpr2gpa PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	MacRomanConversion
	WindowsUnicodeToolShim
	rapidjson/include
	zlib
	)
target_link_libraries(gpr2gpa PortabilityLayer MacRomanConversion zlib)

add_executable(MakeTimestamp MakeTimestamp/MakeTimestamp.cpp)
target_include_directories(MakeTimestamp PRIVATE PortabilityLayer)
target_link_libraries(MakeTimestamp PortabilityLayer)

add_executable(FTagData FTagData/FTagData.cpp WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp)
target_include_directories(FTagData PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	WindowsUnicodeToolShim
	)
target_link_libraries(FTagData PortabilityLayer)

add_executable(MergeGPF
	MergeGPF/MergeGPF.cpp
	AerofoilPortable/GpAllocator_C.cpp
	WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp
	)
target_include_directories(MergeGPF PRIVATE
	Common
	GpCommon
	PortabilityLayer
	AerofoilPortable
	WindowsUnicodeToolShim
	)
target_link_libraries(MergeGPF PortabilityLayer)

add_executable(MiniRez MiniRez/MiniRez.cpp WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp)
target_include_directories(MiniRez PRIVATE
	Common
	GpCommon
	PortabilityLayer
	WindowsUnicodeToolShim
	)
target_link_libraries(MiniRez PortabilityLayer)

add_executable(HouseTool HouseTool/HouseTool.cpp WindowsUnicodeToolShim/UnixUnicodeToolShim.cpp)
target_include_directories(HouseTool PRIVATE
	Common
	GpCommon
	PortabilityLayer
	MacRomanConversion
	WindowsUnicodeToolShim
	)
target_link_libraries(HouseTool PortabilityLayer MacRomanConversion)


install (TARGETS ${EXECNAME})
