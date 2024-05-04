@echo off

set "HOME=%HOMEDRIVE%%HOMEPATH%"

echo Installation Path: [%HOME%/libmelon]
echo Generating files and directories ...

setlocal enabledelayedexpansion

set "old=MLN_ROOT"
set "new=!%HOME%!\libmelon"
set "new=%new:\=\\%"

set "file=src/mln_path.c"

powershell -Command "(Get-Content '%file%') -replace '%old%', '\"%new%\"' | Set-Content '%file%'"


set "old=MLN_NULL"
set "new=!%HOME%!\libmelon\null"
set "new=%new:\=\\%"

set "file=src/mln_path.c"

powershell -Command "(Get-Content '%file%') -replace '%old%', '\"%new%\"' | Set-Content '%file%'"


set "old=MLN_LANG_LIB"
set "new=!%HOME%!\libmelon\lang\lib"
set "new=%new:\=\\%"

set "file=src/mln_path.c"

powershell -Command "(Get-Content '%file%') -replace '%old%', '\"%new%\"' | Set-Content '%file%'"


set "old=MLN_LANG_DYLIB"
set "new=!%HOME%!\libmelon\lang\dylib"
set "new=%new:\=\\%"

set "file=src/mln_path.c"

powershell -Command "(Get-Content '%file%') -replace '%old%', '\"%new%\"' | Set-Content '%file%'"


set "old={{ROOT}}"
set "new=!%HOME%!\libmelon"
set "new=%new:\=\\%"

set "file=conf/melon.conf.msvc.template"

powershell -Command "(Get-Content '%file%') -replace '%old%', '%new%' | Set-Content 'conf/melon.conf'"

endlocal


mkdir lib objs\src "%HOME%\libmelon\logs" "%HOME%\libmelon\lang\lib" "%HOME%\libmelon\lang\dylib" "%HOME%\libmelon\include" "%HOME%\libmelon\conf" "%HOME%\libmelon\lib" "%HOME%\libmelon\tmp"
echo "" > "%HOME%\libmelon\null" 

for %%f in (src\*.c) do (
		cl /c /DMSVC /I include "%%f" /Fo:objs\%%f.o /O2
)

lib /OUT:lib\libmelon_static.lib objs\src\*.o

setlocal

xcopy include "%HOME%/libmelon/include" /E /I /Y
xcopy lib "%HOME%/libmelon/lib" /E /I /Y
xcopy conf "%HOME%/libmelon/conf" /E /I /Y

endlocal

echo Done
