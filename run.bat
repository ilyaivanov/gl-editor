@echo off
set arg1=%1
@REM /Oi /O2
set CompilerOptions=/nologo /GR- /FC /Zi /GS- /Gs9999999

set LinkerOptions=/nodefaultlib /subsystem:windows /STACK:0x100000,0x100000 /incremental:no

set Libs=user32.lib kernel32.lib gdi32.lib opengl32.lib dwmapi.lib

if exist build rmdir /s /q build
mkdir build
pushd build


cl %CompilerOptions% ..\main.c /link %LinkerOptions% %Libs%

echo Compiled

IF NOT "%arg1%" == "b" (
    call .\main.exe
)

popd
