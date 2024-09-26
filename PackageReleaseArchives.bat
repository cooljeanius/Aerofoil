move ReleasePkg\en-us\Aerofoil-installpkg.msi ReleasePkg\Aerofoil-installpkg.msi
cd ReleasePkg
..\Tools\7z.exe a -bd -r -mx=9 Aerofoil.zip Aerofoil
..\Tools\7z.exe a -bd -r -mx=9 AerofoilTools.zip AerofoilTools
..\Tools\7z.exe a -bd -r -mx=9 Aerofoil-PDBs.7z PDBs
cd ..
rmdir /S /Q ReleasePkg\Aerofoil
rmdir /S /Q ReleasePkg\AerofoilTools
rmdir /S /Q ReleasePkg\en-us
rmdir /S /Q ReleasePkg\PDBs

git archive --format zip -o ReleasePkg\Aerofoil-src.zip HEAD
