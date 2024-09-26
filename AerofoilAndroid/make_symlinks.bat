@setlocal enableextensions
@cd /d "%~dp0"

call remove_symlinks.bat


mklink /D app\jni\AerofoilSDL ..\..\..\AerofoilSDL
mklink /D app\jni\AerofoilPortable ..\..\..\AerofoilPortable
mklink /D app\jni\Common ..\..\..\Common
mklink /D app\jni\SDL2 ..\..\..\SDL2-2.0.12
mklink /D app\jni\GpApp ..\..\..\GpApp
mklink /D app\jni\GpShell ..\..\..\GpShell
mklink /D app\jni\GpCommon ..\..\..\GpCommon
mklink /D app\jni\PortabilityLayer ..\..\..\PortabilityLayer
mklink /D app\jni\rapidjson ..\..\..\rapidjson
mklink /D app\jni\MacRomanConversion ..\..\..\MacRomanConversion
mklink /D app\jni\stb ..\..\..\stb

pause
