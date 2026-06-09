@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "CONFIG=Release"
set "BUILD_DIR=build"
set "CMAKE_EXE="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "delims=" %%I in ('where cmake 2^>nul') do (
    if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
)

if not defined CMAKE_EXE (
    if exist "%VSWHERE%" (
        for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`) do (
            if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
        )
    )
)

if not defined CMAKE_EXE (
    echo CMake not found.
    echo Install CMake or Visual Studio CMake tools.
    pause
    exit /b 1
)

echo Using CMake:
echo %CMAKE_EXE%
echo.

"%CMAKE_EXE%" -S . -B "%BUILD_DIR%"
if errorlevel 1 (
    echo CMake configuration failed.
    pause
    exit /b 1
)

"%CMAKE_EXE%" --build "%BUILD_DIR%" --config %CONFIG% --parallel
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
)

if exist "%BUILD_DIR%\_deps\sfml-build\lib\%CONFIG%\*.dll" (
    copy /Y "%BUILD_DIR%\_deps\sfml-build\lib\%CONFIG%\*.dll" "%BUILD_DIR%\%CONFIG%\" >nul
)

echo.
echo Executables found:
dir /s /b "%BUILD_DIR%\*.exe"
echo.
echo Build completed.
pause

endlocal
