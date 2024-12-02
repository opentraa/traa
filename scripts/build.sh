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
    echo "  -U, --unittest          Build unit tests (ON/OFF) [default: OFF]"
    echo "  -S, --smoketest         Build smoke tests (ON/OFF) [default: OFF]"
    echo "  -V, --verbose           Verbose output"
    echo "  -h, --help              Show this help message"
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
            BUILD_UNITTEST="$2"
            shift 2
            ;;
        -S|--smoketest)
            BUILD_SMOKETEST="$2"
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

    case $TARGET_PLATFORM in
        "macos"|"ios"|"xros")
            local cmake_platform
            case $TARGET_PLATFORM in
                "macos") cmake_platform="MAC_UNIVERSAL" ;;
                "ios") cmake_platform="OS64COMBINED" ;;
                "xros") cmake_platform="VISIONOSCOMBINED" ;;
            esac
            
            cmake -G Xcode \
                -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
                -B "$BUILD_FOLDER" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DPLATFORM="$cmake_platform" \
                -DTRAA_OPTION_BIN_FOLDER="$BIN_FOLDER" \
                -DTRAA_OPTION_ENABLE_UNIT_TEST="$BUILD_UNITTEST" \
                -DTRAA_OPTION_ENABLE_SMOKE_TEST="$BUILD_SMOKETEST" \
                -DTRAA_OPTION_VERSION="$VERSION" \
                -S "$SOURCE_DIR"
            ;;
        "linux")
            cmake -B "$BUILD_FOLDER" \
                -DCMAKE_CXX_COMPILER=g++ \
                -DCMAKE_C_COMPILER=gcc \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DTRAA_OPTION_BIN_FOLDER="$BIN_FOLDER" \
                -DTRAA_OPTION_ENABLE_UNIT_TEST="$BUILD_UNITTEST" \
                -DTRAA_OPTION_ENABLE_SMOKE_TEST="$BUILD_SMOKETEST" \
                -DTRAA_OPTION_VERSION="$VERSION" \
                -S "$SOURCE_DIR"
            ;;
        "android")
            log "WARN" "Android build not implemented yet"
            exit 1
            ;;
        *)
            log "ERROR" "Unsupported platform: $platform"
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
}

# execute main build function
build