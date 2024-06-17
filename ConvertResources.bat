rmdir /S /Q Packaged

mkdir Packaged
mkdir Packaged\Houses

x64\Release\MiniRez.exe "GliderProData\Glider PRO.r" Packaged\ApplicationResources.gpr
x64\Release\gpr2gpa.exe "Packaged\ApplicationResources.gpr" "DefaultTimestamp.timestamp" "Packaged\ApplicationResources.gpa" -patch "ApplicationResourcePatches\manifest.json"
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\ApplicationResources.gpf" data ozm5 0 0 locked
x64\Release\MergeGPF.exe "Packaged\ApplicationResources.gpf"

x64\Release\GenerateFonts.exe Resources Packaged

x64\Release\MiniRez.exe "Empty.r" Packaged\Fonts.gpr
x64\Release\gpr2gpa.exe "Packaged\Fonts.gpr" "DefaultTimestamp.timestamp" "Packaged\Fonts.gpa" -patch "Packaged\FontCacheManifest.json"
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Fonts.gpf" data ozm5 0 0 locked
x64\Release\MergeGPF.exe "Packaged\Fonts.gpf"

del /Q Packaged\CachedFont*.bin
del Packaged\FontCacheCatalog.bin
del Packaged\FontCacheManifest.json
del Packaged\Fonts.gpr
del Packaged\Fonts.gpa

x64\Release\ConvertColorCursors.exe

attrib -R Packaged\ApplicationResources.gpf
attrib -R Packaged\Houses\*

x64\Release\hqx2gp.exe "GliderProData\Houses\Art Museum.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Art Museum"
x64\Release\hqx2gp.exe "GliderProData\Houses\California or Bust!.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\California or Bust!"
x64\Release\hqx2gp.exe "GliderProData\Houses\Castle o' the Air.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Castle o' the Air"
x64\Release\hqx2gp.exe "GliderProData\Houses\CD Demo House.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\CD Demo House"
x64\Release\hqx2gp.exe "GliderProData\Houses\Davis Station.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Davis Station"
x64\Release\hqx2gp.exe "GliderProData\Houses\Demo House.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Demo House"
x64\Release\hqx2gp.exe "GliderProData\Houses\Fun House.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Fun House"
x64\Release\hqx2gp.exe "GliderProData\Houses\Grand Prix.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Grand Prix"
x64\Release\hqx2gp.exe "GliderProData\Houses\ImagineHouse PRO II.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\ImagineHouse PRO II"
x64\Release\hqx2gp.exe "GliderProData\Houses\In The Mirror.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\In The Mirror"
x64\Release\hqx2gp.exe "GliderProData\Houses\Land of Illusion.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Land of Illusion"
x64\Release\hqx2gp.exe "GliderProData\Houses\Leviathan.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Leviathan"
x64\Release\hqx2gp.exe "GliderProData\Houses\Metropolis.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Metropolis"
x64\Release\hqx2gp.exe "GliderProData\Houses\Nemo's Market.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Nemo's Market"
x64\Release\hqx2gp.exe "GliderProData\Houses\Rainbow's End.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Rainbow's End"
x64\Release\hqx2gp.exe "GliderProData\Houses\Slumberland.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Slumberland"
x64\Release\hqx2gp.exe "GliderProData\Houses\SpacePods.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\SpacePods"
x64\Release\hqx2gp.exe "GliderProData\Houses\Teddy World.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Teddy World"
x64\Release\hqx2gp.exe "GliderProData\Houses\The Asylum Pro.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\The Asylum Pro"
x64\Release\hqx2gp.exe "GliderProData\Houses\Titanic.binhex" "DefaultTimestamp.timestamp" "Packaged\Houses\Titanic"

x64\Release\gpr2gpa.exe "Packaged\Houses\Art Museum.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Art Museum.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\California or Bust!.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\California or Bust!.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Castle o' the Air.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Castle o' the Air.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\CD Demo House.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\CD Demo House.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Davis Station.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Davis Station.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Demo House.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Demo House.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Fun House.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Fun House.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Grand Prix.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Grand Prix.gpa" -patch "HousePatches\GrandPrix.json"
x64\Release\gpr2gpa.exe "Packaged\Houses\ImagineHouse PRO II.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\ImagineHouse PRO II.gpa" -patch "HousePatches\ImagineHousePROII.json"
x64\Release\gpr2gpa.exe "Packaged\Houses\In The Mirror.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\In The Mirror.gpa" -patch "HousePatches\InTheMirror.json"
x64\Release\gpr2gpa.exe "Packaged\Houses\Land of Illusion.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Land of Illusion.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Leviathan.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Leviathan.gpa" -patch "HousePatches\Leviathan.json"
x64\Release\gpr2gpa.exe "Packaged\Houses\Metropolis.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Metropolis.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Nemo's Market.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Nemo's Market.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Rainbow's End.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Rainbow's End.gpa" -patch "HousePatches\RainbowsEnd.json"
x64\Release\gpr2gpa.exe "Packaged\Houses\Slumberland.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Slumberland.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\SpacePods.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\SpacePods.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Teddy World.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Teddy World.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\The Asylum Pro.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\The Asylum Pro.gpa"
x64\Release\gpr2gpa.exe "Packaged\Houses\Titanic.gpr" "DefaultTimestamp.timestamp" "Packaged\Houses\Titanic.gpa"

x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Art Museum.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Castle o' the Air.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\CD Demo House.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Davis Station.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Demo House.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Grand Prix.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\ImagineHouse PRO II.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Land of Illusion.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Leviathan.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Nemo's Market.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Rainbow's End.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Slumberland.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\SpacePods.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Teddy World.mov.gpf" MooV ozm5 0 0 locked
x64\Release\FTagData.exe "DefaultTimestamp.timestamp" "Packaged\Houses\Titanic.mov.gpf" MooV ozm5 0 0 locked

copy /Y GliderProData\ConvertedMovies\*.mov.gpa Packaged\Houses\

rem Patch bad Castle 'o the Air start room
x64\Release\HouseTool.exe patch "Packaged\Houses\Castle o' the Air.gpd" .firstRoom 77

x64\Release\MergeGPF.exe "Packaged\Houses\Art Museum.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\California or Bust!.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Castle o' the Air.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\CD Demo House.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Davis Station.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Demo House.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Fun House.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Grand Prix.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\ImagineHouse PRO II.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\In The Mirror.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Land of Illusion.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Leviathan.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Metropolis.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Nemo's Market.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Rainbow's End.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Slumberland.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\SpacePods.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Teddy World.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\The Asylum Pro.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Titanic.gpf"

x64\Release\MergeGPF.exe "Packaged\Houses\Art Museum.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Castle o' the Air.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\CD Demo House.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Davis Station.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Demo House.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Grand Prix.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\ImagineHouse PRO II.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Land of Illusion.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Leviathan.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Nemo's Market.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Rainbow's End.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Slumberland.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\SpacePods.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Teddy World.mov.gpf"
x64\Release\MergeGPF.exe "Packaged\Houses\Titanic.mov.gpf"

del /Q Packaged\Houses\*.gpr
del /Q Packaged\Houses\*.gpa
del /Q Packaged\Houses\*.gpd
del /Q Packaged\ApplicationResources.gpr
del /Q Packaged\ApplicationResources.gpa

attrib +R Packaged\Houses\*
attrib +R Packaged\ApplicationResources.gpf

pause
