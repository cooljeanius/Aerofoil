set INPUT_DIR=../GpShell
set OUTPUT_DIR=obj

rem set DEBUG_LEVEL_FLAGS=-g4 -O0
set DEBUG_LEVEL_FLAGS=-O3 -DNDEBUG=1

set FLAGS=-s USE_SDL=2 -flto -I../GpCommon/ -I../PortabilityLayer/ -I../Common/ -s ASYNCIFY %DEBUG_LEVEL_FLAGS% -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/GpShell_Combined.cpp -o %OUTPUT_DIR%/GpShell_Combined.o %FLAGS%

rem pause
