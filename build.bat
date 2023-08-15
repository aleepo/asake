@echo off

set CC=cl
set SHARED_FLAGS=/nologo /std:c11 /W3 /FC
set BUILD_DIR=bin

if "%~1" == "release" (
    set args=release
) else (
    set args=debug
)

if %args% == release (
   echo "Release Build"
   rem set BUILD_DIR=release_build
   set BUILD_ARGS=/O2 /Os /Feasake_release
) else (
   echo "Debug Build"
   rem set BUILD_DIR=debug_build
   set BUILD_ARGS=/Z7 /Feasake_debug
)

RD /S /Q %BUILD_DIR%
mkdir %BUILD_DIR%
pushd %BUILD_DIR%

REM Compile resources first
echo ------------------------------
echo Building Resources
rc /nologo ..\src\resource.rc

%CC% %SHARED_FLAGS% %BUILD_ARGS% ..\src\win32_asake.c kernel32.lib user32.lib gdi32.lib Shcore.lib Xinput9_1_0.lib /link ..\src\resource.res
popd
