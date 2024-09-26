cd ..
copy /Y DefaultTimestamp.timestamp AerofoilAndroid\app\src\main\assets\Packaged\DefaultTimestamp.timestamp
del AerofoilAndroid\app\src\main\assets\Packaged\SourceCode.zip
del AerofoilAndroid\app\src\main\assets\Packaged\SourceCode.pkg
git archive -0 --format zip -o AerofoilAndroid\app\src\main\assets\Packaged\SourceCode.zip HEAD
tools\7z.exe d AerofoilAndroid\app\src\main\assets\Packaged\SourceCode.zip GliderProData\
cd AerofoilAndroid\app\src\main\assets\Packaged
rename SourceCode.zip SourceCode.pkg
pause
