set INPUT_DIR=../PortabilityLayer
set OUTPUT_DIR=obj

rem set DEBUG_LEVEL_FLAGS=-g4 -O0
set DEBUG_LEVEL_FLAGS=-O3 -DNDEBUG=1

set FLAGS=-s USE_ZLIB=1 -flto -I../GpCommon/ -I../Common/ -I../PortabilityLayer/ -I../rapidjson/include/ -I../MacRomanConversion/ -I../stb/ -s ASYNCIFY %DEBUG_LEVEL_FLAGS% -DGP_DEBUG_CONFIG=0 -Wno-tautological-constant-out-of-range-compare

emcc -c %INPUT_DIR%/PortabilityLayer_Combined.cpp -o %OUTPUT_DIR%/PortabilityLayer_Combined.o %FLAGS%
bash emcc_docker.sh -c %INPUT_DIR%/PortabilityLayer_Combined.cpp -o %OUTPUT_DIR%/PortabilityLayer_Combined.o %FLAGS%

rem pause
sleep 1
