cd app
cd src
cd main
cd assets
rmdir /S /Q Packaged
mkdir Packaged
cd Packaged
rmdir /S /Q Houses
mkdir Houses
copy ..\..\..\..\..\..\Packaged\*.gpf .\
copy ..\..\..\..\..\..\Packaged\Houses\* Houses\
cd ..
