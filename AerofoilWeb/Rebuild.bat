call MakeBuildDirs.bat
nuget install Microsoft.NET.Runtime.Emscripten.2.0.23.Sdk.win-x64
yarn add emscripten quickjs-emscripten
call BuildAerofoilSDL.bat
call BuildAerofoilWeb.bat
call BuildGpApp.bat
call BuildGpShell.bat
call BuildMacRomanConversion.bat
call BuildPortabilityLayer.bat
call BuildResources.bat
call Link.bat
