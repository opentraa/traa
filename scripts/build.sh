#!/bin/bash

# colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# default values
BUILD_TYPE="Release"
BUILD_FOLDER="$PWD/build"
BIN_FOLDER="$PWD/bin"
SOURCE_DIR="$PWD"
TARGET_PLATFORM=""
VERSION="1.0.0"
VERBOSE=0
BUILD_UNITTEST=OFF
BUILD_SMOKETEST=OFF
ANDROID_ABI="arm64-v8a,armeabi-v7a,x86,x86_64"
TARGET_ARCH="x86_64"
NO_FRAMEWORK=OFF

# cpu_features library on Android implements getauxval() for API level < 18, see cpu_features/src/hwcaps_linux_or_android.c
# The implementation call dlopen() to load libc.so and then call dlsym() to get the address of getauxval().
# Which is not allowed in most Android apps, so we need to set the minimum API level to 18.
ANDROID_API_LEVEL="18"

# function for logging
log() {
    local level=$1
    shift
    case $level in
        "INFO") echo -e "${GREEN}[INFO]${NC} $*" ;;
        "WARN") echo -e "${YELLOW}[WARN]${NC} $*" ;;
        "ERROR") echo -e "${RED}[ERROR]${NC} $*" ;;
    esac
}

# function to show usage
show_usage() {
    echo "Usage: $0 -p <platform> [options]"
    echo "Options:"
    echo "  -p, --platform          Target platform (macos/ios/xros/linux/android) [required]"
    echo "  -t, --build-type        Build type (Debug/Release) [default: Release]"
    echo "  -f, --build-folder      Build folder path [default: build]"
    echo "  -b, --bin-folder        Binary output folder [default: bin]"
    echo "  -s, --source-dir        Source directory [default: current directory]"
    echo "  -v, --version           Version of the build [default: 1.0.0]"
    echo "  -U, --unittest          Build unit tests"
    echo "  -S, --smoketest         Build smoke tests"
    echo "  -A, --android-abi       Android ABI [default: arm64-v8a,armeabi-v7a,x86,x86_64]"
    echo "  -V, --verbose           Verbose output"
    echo "  -h, --help              Show this help message"
    echo "  -L, --api-level         Android API level [default: 18]"
    echo "  -a, --arch              Target architecture for Linux (x86_64/aarch64_clang/aarch64_gnu) [default: x86_64]"
    echo "      --no-framework      Do not build framework for Apple platforms"
}

# parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -f|--build-folder)
            BUILD_FOLDER="$2"
            shift 2
            ;;
        -b|--bin-folder)
            BIN_FOLDER="$2"
            shift 2
            ;;
        -p|--platform)
            TARGET_PLATFORM="$2"
            shift 2
            ;;
        -s|--source-dir)
            SOURCE_DIR="$2"
            shift 2
            ;;
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -U|--unittest)
            BUILD_UNITTEST="ON"
            shift
            ;;
        -S|--smoketest)
            BUILD_SMOKETEST="ON"
            shift
            ;;
        -A|--android-abi)
            ANDROID_ABI="$2"
            shift 2
            ;;
        -V|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        -L|--api-level)
            ANDROID_API_LEVEL="$2"
            shift 2
            ;;
        -a|--arch)
            TARGET_ARCH="$2"
            shift 2
            ;;
        --no-framework)
            NO_FRAMEWORK="ON"
            shift
            ;;
        *)
            log "ERROR" "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# validate required parameters
if [ -z "$TARGET_PLATFORM" ]; then
    log "ERROR" "Target platform is required"
    show_usage
    exit 1
fi

# validate build type
if [[ ! "$BUILD_TYPE" =~ ^(Debug|Release)$ ]]; then
    log "ERROR" "Invalid build type: $BUILD_TYPE"
    exit 1
fi

# validate target platform
if [[ ! "$TARGET_PLATFORM" =~ ^(macos|ios|xros|linux|android)$ ]]; then
    log "ERROR" "Invalid target platform: $TARGET_PLATFORM"
    exit 1
fi

# Add architecture validation for Linux platform
if [ "$TARGET_PLATFORM" == "linux" ]; then
    if [[ ! "$TARGET_ARCH" =~ ^(x86_64|aarch64_clang|aarch64_gnu)$ ]]; then
        log "ERROR" "Invalid Linux architecture: $TARGET_ARCH"
        exit 1
    fi
fi

# main build function
build() {
    log "INFO" "Starting build for platform: $TARGET_PLATFORM"
    log "INFO" "Build type: $BUILD_TYPE"
    log "INFO" "Build folder: $BUILD_FOLDER"
    log "INFO" "Binary folder: $BIN_FOLDER"
    log "INFO" "Source directory: $SOURCE_DIR"
    log "INFO" "Version: $VERSION"

    BUILD_FOLDER="$BUILD_FOLDER/$TARGET_PLATFORM"
    log "INFO" "Redirect build folder: $BUILD_FOLDER"

    BIN_FOLDER="$BIN_FOLDER/$TARGET_PLATFORM"
    log "INFO" "Redirect binary folder: $BIN_FOLDER"

    # configure cmake
    log "INFO" "Configuring CMake..."
    # create build directory if it doesn't exist
    mkdir -p "$BUILD_FOLDER"
    mkdir -p "$BIN_FOLDER"

    # if target platform is android, we should run cmake for each ABI separately
    if [ "$TARGET_PLATFORM" == "android" ]; then
        log "INFO" "Android ABI: $ANDROID_ABI"
        for abi in $(echo $ANDROID_ABI | tr "," "\n");do
            log "INFO" "Building for ABI: $abi"
            local build_folder_abi="$BUILD_FOLDER/$abi"
            local bin_folder_abi="$BIN_FOLDER" # no need to create separate bin folder for each ABI
            mkdir -p "$build_folder_abi"
            mkdir -p "$bin_folder_abi"

            cmake -B "$build_folder_abi" \
                -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
                -DANDROID_ABI="$abi" \
                -DANDROID_NDK="$ANDROID_NDK_HOME" \
                -DCMAKE_SYSTEM_NAME=Android \
                -DANDROID_PLATFORM=android-$ANDROID_API_LEVEL \
                -DANDROID_STL=c++_static \
                -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DTRAA_OPTION_BIN_FOLDER="$bin_folder_abi" \
                -DTRAA_OPTION_ENABLE_UNIT_TEST="$BUILD_UNITTEST" \
                -DTRAA_OPTION_ENABLE_SMOKE_TEST="$BUILD_SMOKETEST" \
                -DTRAA_OPTION_VERSION="$VERSION" \
                -S "$SOURCE_DIR"

            if [ $? -ne 0 ]; then
                log "ERROR" "CMake failed for ABI: $abi"
                exit 1
            fi

            log "INFO" "Building..."
            if [ $VERBOSE -eq 1 ]; then
                cmake --build "$build_folder_abi" --config "$BUILD_TYPE" -v
            else
                cmake --build "$build_folder_abi" --config "$BUILD_TYPE"
            fi

            if [ $? -ne 0 ]; then
                log "ERROR" "Build failed for ABI: $abi"
                exit 1
            fi
        done

        # build gradle project
        log "INFO" "Building Gradle project..."
        pushd "projects/android"
        sh ./gradlew clean assemble$BUILD_TYPE
        sh ./gradlew assemble$BUILD_TYPE
        popd

        # copy aar file to bin folder
        lower_build_type=$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')
        cp "./projects/android/traa/build/outputs/aar/traa-$lower_build_type.aar" "$BIN_FOLDER/traa.aar"

        # copy jar file to bin folder from sync folder like projects/android/build/intermediates/aar_main_jar/debug/syncDebugLibJars
        cp "./projects/android/traa/build/intermediates/aar_main_jar/$lower_build_type/sync${BUILD_TYPE}LibJars/classes.jar" "$BIN_FOLDER/traa.jar"

        if [ $? -eq 0 ]; then
            log "INFO" "Build completed successfully"
        else
            log "ERROR" "Build failed"
            exit 1
        fi
    else
        case $TARGET_PLATFORM in
        "macos"|"ios"|"xros")
            local cmake_platform

            # TODO @sylar: need to build OS64COMBINED and SIMULATOR64 for ios and then create xcframework
            case $TARGET_PLATFORM in
                "macos") cmake_platform="MAC_UNIVERSAL" ;;
                "ios") cmake_platform="OS64COMBINED" ;;
                "xros") cmake_platform="VISIONOSCOMBINED" ;;
            esac
            
            cmake -G Xcode \
                -DCMAKE_TOOLCHAIN_FILE=cmake/macos/ios.toolchain.cmake \
                -B "$BUILD_FOLDER" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DPLATFORM="$cmake_platform" \
                -DTRAA_OPTION_BIN_FOLDER="$BIN_FOLDER" \
                -DTRAA_OPTION_ENABLE_UNIT_TEST="$BUILD_UNITTEST" \
                -DTRAA_OPTION_ENABLE_SMOKE_TEST="$BUILD_SMOKETEST" \
                -DTRAA_OPTION_VERSION="$VERSION" \
                -DTRAA_OPTION_NO_FRAMEWORK="$NO_FRAMEWORK" \
                -S "$SOURCE_DIR"
            ;;
        "linux")
            local toolchain_file
            case $TARGET_ARCH in
                "x86_64")
                    toolchain_file="cmake/linux/toolchain-x86_64-gcc.cmake"
                    ;;
                "aarch64_clang")
                    toolchain_file="cmake/linux/toolchain-aarch64-clang.cmake"
                    ;;
                "aarch64_gnu")
                    toolchain_file="cmake/linux/toolchain-aarch64-gcc.cmake"
                    ;;
            esac

            log "INFO" "Using toolchain file: $toolchain_file"
            log "INFO" "Building for architecture: $TARGET_ARCH"

            # set sub folder for BUILD_FOLDER
            BUILD_FOLDER="$BUILD_FOLDER/$TARGET_ARCH"

            cmake -B "$BUILD_FOLDER" \
                -DCMAKE_TOOLCHAIN_FILE="$toolchain_file" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DTRAA_OPTION_BIN_FOLDER="$BIN_FOLDER" \
                -DTRAA_OPTION_ENABLE_UNIT_TEST="$BUILD_UNITTEST" \
                -DTRAA_OPTION_ENABLE_SMOKE_TEST="$BUILD_SMOKETEST" \
                -DTRAA_OPTION_VERSION="$VERSION" \
                -S "$SOURCE_DIR"
            ;;
        *)
            log "ERROR" "Unsupported platform: $TARGET_PLATFORM"
            exit 1
            ;;
        esac

        # build
        log "INFO" "Building..."
        if [ $VERBOSE -eq 1 ]; then
            cmake --build "$BUILD_FOLDER" --config "$BUILD_TYPE" -v
        else
            cmake --build "$BUILD_FOLDER" --config "$BUILD_TYPE"
        fi

        if [ $? -eq 0 ]; then
            log "INFO" "Build completed successfully"
        else
            log "ERROR" "Build failed"
            exit 1
        fi
    fi
}

# execute main build function
build