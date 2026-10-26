@echo off
REM MiniUT2 build script - smallest working release.
REM Uses MSVC (cl). Calls vcvars64.bat automatically if cl is not already on PATH.
REM Smallest-size flags: /O2 /Os /Gy + linker /OPT:ICF /OPT:REF (dedup identical functions/data).
cd /d "%~dp0src"

where cl >nul 2>nul
if %errorlevel%==0 goto :build

REM cl not on PATH: locate and call vcvars64.bat.
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" goto :havevars
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" goto :havevars
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" goto :havevars
echo cl not found and vcvars64.bat not located. Run from a Developer Command Prompt.
exit /b 1

:havevars
call "%VCVARS%" >nul

:build
echo Building smallest release with MSVC...
cl /nologo /EHsc /O2 /Os /Gy /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE ^
   main.cpp Package.cpp WorldBuilder.cpp Pawn.cpp Texture.cpp ^
   /Fe:..\miniut2.exe /Fo:..\ ^
   /link /OPT:ICF /OPT:REF opengl32.lib user32.lib gdi32.lib
if errorlevel 1 (
    echo Build FAILED.
    exit /b 1
)

REM Remove intermediate object/ltcg files so the folder stays minimal.
del /q ..\*.obj ..\*.ilk ..\*.pdb 2>nul
echo Build OK: ..\miniut2.exe
