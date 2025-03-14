@echo off
setlocal EnableDelayedExpansion

:: Enable ANSI escape codes
for /f "tokens=2 delims=: " %%i in ('ver') do set "ver=%%i"
if "%ver:~0,2%" geq "10" (
    set "ENABLE_ANSI=1"
) else (
    set "ENABLE_ANSI=0"
)

:: Colors for output
if "%ENABLE_ANSI%"=="1" (
    set "RED=[0;31m"
    set "GREEN=[0;32m"
    set "YELLOW=[1;33m"
    set "NC=[0m"
) else (
    set "RED="
    set "GREEN="
    set "YELLOW="
    set "NC="
)

:: Default values
set "BUILD_ARCH=x64"
set "BUILD_TYPE=Release"
set "BUILD_FOLDER=%cd%\build\win"
set "BIN_FOLDER=%cd%\bin"
set "SOURCE_DIR=%cd%"
set "VERSION=0.0.1"
set "VERBOSE=0"
set "BUILD_UNITTEST=OFF"
set "BUILD_SMOKETEST=OFF"

:: Execute main build function
call :parse_args %*

:: Validate build type
if /i not "%BUILD_TYPE%"=="Debug" if /i not "%BUILD_TYPE%"=="Release" (
    call :log ERROR "Invalid build type: %BUILD_TYPE%"
    exit /b 1
)

call :build
exit /b 0

:: Function for logging
:log
    set "level=%1"
    shift
    if "%level%"=="INFO" (
        echo %GREEN%[INFO]%NC% %*
    ) else if "%level%"=="WARN" (
        echo %YELLOW%[WARN]%NC% %*
    ) else if "%level%"=="ERROR" (
        echo %RED%[ERROR]%NC% %*
    )
goto:eof

:: Function to show usage
:show_usage
    echo Usage: build.bat [options]
    echo Options:
    echo   -a, --arch              Architecture (Win32/x64/ARM64) [default: x64]
    echo   -t, --build-type        Build type (Debug/Release) [default: Release]
    echo   -f, --build-folder      Build folder path [default: build]
    echo   -b, --bin-folder        Binary output folder [default: bin]
    echo   -s, --source-dir        Source directory [default: current directory]
    echo   -v, --version           Version number [default: 0.0.1]
    echo   -V, --verbose           Verbose output
    echo   -U, --unittest          Build unit tests
    echo   -S, --smoketest         Build smoke tests
    echo   -h, --help              Show this help message
    exit 0
goto:eof

:: Parse command line arguments
:parse_args
    if "%~1"=="" goto end_parse
    set "arg=%~1"
    shift
    if "%arg%"=="-a" (
        set "BUILD_ARCH=%~1"
        shift
    ) else if "%arg%"=="--arch" (
        set "BUILD_ARCH=%~1"
        shift
    ) else if "%arg%"=="-t" (
        set "BUILD_TYPE=%~1"
        shift
    ) else if "%arg%"=="--build-type" (
        set "BUILD_TYPE=%~1"
        shift
    ) else if "%arg%"=="-f" (
        set "BUILD_FOLDER=%~1"
        shift
    ) else if "%arg%"=="--build-folder" (
        set "BUILD_FOLDER=%~1"
        shift
    ) else if "%arg%"=="-b" (
        set "BIN_FOLDER=%~1"
        shift
    ) else if "%arg%"=="--bin-folder" (
        set "BIN_FOLDER=%~1"
        shift
    ) else if "%arg%"=="-s" (
        set "SOURCE_DIR=%~1"
        shift
    ) else if "%arg%"=="--source-dir" (
        set "SOURCE_DIR=%~1"
        shift
    ) else if "%arg%"=="-v" (
        set "VERSION=%~1"
        shift
    ) else if "%arg%"=="--version" (
        set "VERSION=%~1"
        shift
    ) else if "%arg%"=="-V" (
        set "VERBOSE=1"
    ) else if "%arg%"=="--verbose" (
        set "VERBOSE=1"
    ) else if "%arg%"=="-U" (
        set "BUILD_UNITTEST=ON"
    ) else if "%arg%"=="--unittest" (
        set "BUILD_UNITTEST=ON"
    ) else if "%arg%"=="-S" (
        set "BUILD_SMOKETEST=ON"
    ) else if "%arg%"=="--smoketest" (
        set "BUILD_SMOKETEST=ON"
    ) else if "%arg%"=="-h" (
        call :show_usage
    ) else (
        call :log ERROR "Unknown option: %arg%"
        call :show_usage
    )
    goto parse_args
:end_parse
goto:eof

:: Main build function
:build
    call :log INFO "Build type: %BUILD_TYPE%"
    call :log INFO "Build folder: %BUILD_FOLDER%"
    call :log INFO "Binary folder: %BIN_FOLDER%"

    set "BUILD_FOLDER=%BUILD_FOLDER%\%BUILD_ARCH%"
    call :log INFO "Redirect Build folder: %BUILD_FOLDER%"

    :: Create build and binary directories if they don't exist
    mkdir "%BUILD_FOLDER%"
    mkdir "%BIN_FOLDER%"

    :: Configure CMake
    call :log INFO "Configuring CMake..."

    cmake -A "%BUILD_ARCH%" ^
        -B "%BUILD_FOLDER%" ^
        -DCMAKE_BUILD_TYPE="%BUILD_TYPE%" ^
        -DTRAA_OPTION_BIN_FOLDER="%BIN_FOLDER%" ^
        -DTRAA_OPTION_ENABLE_UNIT_TEST="%BUILD_UNITTEST%" ^
        -DTRAA_OPTION_ENABLE_SMOKE_TEST="%BUILD_SMOKETEST%" ^
        -DTRAA_OPTION_VERSION="%VERSION%" ^
        -S "%SOURCE_DIR%"

    :: Build
    call :log INFO "Building..."
    if "%VERBOSE%"=="1" (
        cmake --build "%BUILD_FOLDER%" --config "%BUILD_TYPE%" --verbose
    ) else (
        cmake --build "%BUILD_FOLDER%" --config "%BUILD_TYPE%"
    )

    if %errorlevel%==0 (
        call :log INFO "Build completed successfully"
        exit /b 0
    ) else (
        call :log ERROR "Build failed"
        exit /b 1
    )
goto:eof

endlocal