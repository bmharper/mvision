@echo off

rem We could use a custom tundra rule for this, but I'm being lazy here
rem This technique is described at http://wiki.videolan.org/GenerateLibFromDll

tundra2 debug release

echo EXPORTS > libccv.def

rem ---------------------------------------------------------------------------------------------------------------------------------------------------------
for /f "usebackq tokens=4,* delims=_ " %%i in (`dumpbin /exports "t2-output\win64-mingw-debug-default\libccv.dll"`) do if %%i==ccv echo %%i_%%j >> libccv.def
rem                                                                                                                             ^
rem ----------------------------------------------------------------------------------------------------------------------------prefix-----------------------

lib /def:"libccv.def" /out:"t2-output\win64-mingw-debug-default\libccv.lib" /machine:x64
lib /def:"libccv.def" /out:"t2-output\win64-mingw-release-default\libccv.lib" /machine:x64

