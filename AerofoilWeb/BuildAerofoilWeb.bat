set INPUT_DIR=.
set OUTPUT_DIR=obj

rem set DEBUG_LEVEL_FLAGS=-g4 -O0
set DEBUG_LEVEL_FLAGS=-O3 -DNDEBUG=1

set FLAGS=-s USE_SDL=2 -flto -I../GpCommon/ -I../PortabilityLayer/ -I../Common/ -I../AerofoilPortable/ -I../GpShell/ -s ASYNCIFY %DEBUG_LEVEL_FLAGS% -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/AerofoilWeb_Combined.cpp -o %OUTPUT_DIR%/AerofoilWeb_Combined.o %FLAGS%

rem pause
