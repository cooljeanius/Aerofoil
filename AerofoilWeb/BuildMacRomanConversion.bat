set INPUT_DIR=../MacRomanConversion
set OUTPUT_DIR=obj

rem set DEBUG_LEVEL_FLAGS=-g4 -O0
set DEBUG_LEVEL_FLAGS=-O3 -DNDEBUG=1

set FLAGS=-flto -I../MacRomanConversion/ -s ASYNCIFY %DEBUG_LEVEL_FLAGS%

emcc -c %INPUT_DIR%/MacRomanConversion.cpp -o %OUTPUT_DIR%/MacRomanConversion.o %FLAGS%
bash emcc_docker.sh -c %INPUT_DIR%/MacRomanConversion.cpp -o %OUTPUT_DIR%/MacRomanConversion.o %FLAGS%

rem pause
sleep 1
