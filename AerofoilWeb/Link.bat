set INPUT_DIR=.
set OUTPUT_DIR=bin

rem set DEBUG_LEVEL_FLAGS=-g4 -O0
set DEBUG_LEVEL_FLAGS=-O3

copy /Y FileSaverDotJS\dist\FileSaver.js bin\FileSaver.js

set FLAGS=-flto %DEBUG_LEVEL_FLAGS% -s USE_SDL=2 -s USE_ZLIB=1 -s ASYNCIFY -s ASYNCIFY_IGNORE_INDIRECT -s INITIAL_MEMORY=33554432 -s ASYNCIFY_ADVISE -lidbfs.js -s ASYNCIFY_IMPORTS=['InitFileSystem','FlushFileSystem'] --shell-file shell_minimal.html
emcc obj/AerofoilWeb_Combined.o obj/AerofoilWeb_Resources.o obj/GpShell_Combined.o obj/AerofoilSDL_Combined.o obj/AerofoilPortable_Combined.o obj/GpApp_Combined.o obj/PortabilityLayer_Combined.o obj/MacRomanConversion.o -o %OUTPUT_DIR%/aerofoil.html %FLAGS%
bash emcc_docker.sh obj/AerofoilWeb_Combined.o obj/AerofoilWeb_Resources.o obj/GpShell_Combined.o obj/AerofoilSDL_Combined.o obj/AerofoilPortable_Combined.o obj/GpApp_Combined.o obj/PortabilityLayer_Combined.o obj/MacRomanConversion.o -o %OUTPUT_DIR%/aerofoil.html %FLAGS%
