@echo off
setlocal

set BUILD_DIR=build
set CONFIG=Debug

if not "%~1"=="" set BUILD_DIR=%~1
if not "%~2"=="" set CONFIG=%~2

set REPO_ROOT=%~dp0..\..
for %%I in ("%REPO_ROOT%") do set REPO_ROOT=%%~fI
set BUILD_PATH=%REPO_ROOT%\%BUILD_DIR%

echo Configuring CMake in %BUILD_PATH% ...
cmake -S "%REPO_ROOT%" -B "%BUILD_PATH%" -DNFCCPP_BUILD_EXAMPLES=ON
if errorlevel 1 exit /b %errorlevel%

echo Building desfire_get_file_settings_example (%CONFIG%) ...
cmake --build "%BUILD_PATH%" --target desfire_get_file_settings_example --config %CONFIG%
exit /b %errorlevel%

