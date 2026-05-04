@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo VCVARS_FAILED & exit /b 1 )
cmake --build out\build\x64-Debug --target AutoDetect
if errorlevel 1 ( exit /b 1 )
cmake --build out\build\x64-Debug --target GentzenW
