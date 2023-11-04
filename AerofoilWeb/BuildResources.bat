..\x64\Release\bin2h.exe ..\Packaged res\ApplicationData.h
..\x64\Release\bin2h.exe ..\Packaged\Houses res\GameData.h

set INPUT_DIR=.
set OUTPUT_DIR=obj
set FLAGS=-flto -O3 -DGP_DEBUG_CONFIG=0

emcc -c %INPUT_DIR%/AerofoilWeb_Resources.cpp -o %OUTPUT_DIR%/AerofoilWeb_Resources.o %FLAGS%
bash emcc_docker.sh -c %INPUT_DIR%/AerofoilWeb_Resources.cpp -o %OUTPUT_DIR%/AerofoilWeb_Resources.o %FLAGS%

rem pause
sleep 1
