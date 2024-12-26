# traa
[![Build Status](https://github.com/opentraa/traa/actions/workflows/ci-pr-on-main.yml/badge.svg)](https://github.com/opentraa/traa/actions)
![GitHub stars](https://img.shields.io/github/stars/opentraa/traa)
![GitHub forks](https://img.shields.io/github/forks/opentraa/traa)
![GitHub repo size](https://img.shields.io/github/repo-size/opentraa/traa)
![GitHub contributors](https://img.shields.io/github/contributors/opentraa/traa)
![GitHub last commit](https://img.shields.io/github/last-commit/opentraa/traa)

## Introduction

`traa` is a versatile project aimed at recording anything, anywhere. The primary focus is to provide robust solutions for various recording scenarios, making it a highly adaptable tool for multiple use cases.

## Vision

Our ultimate goal is to create a very small but feature-rich dynamic library for audio and video capture, processing, and display. This will allow audio and video developers to easily integrate it into their projects. In the future, we aim to incorporate AI capabilities to enhance audio and video processing.

## Contribution

We welcome contributions from the community. Feel free to open issues or submit pull requests to help improve `traa`.

## Motivation

If you find this project useful, a star on GitHub would be greatly appreciated. Your support motivates us to keep improving and adding new features.

## Implemented Features

- **ASIO-based Asynchronous Threading Model**: The project includes a task timer that executes tasks repeatedly at specified intervals using `asio::io_context`. This model ensures efficient task scheduling and execution.
- **Screen Source Enumeration**: The project provides functionality for enumerating screen sources on Windows and macOS, retrieving screen source information such as icon size and thumbnail size.
- **Screen Capture On Windows**: The project provides functionality for capturing screen on Windows.

## Unimplemented Features

- **Audio Device Management (ADM)**: This module will handle the enumeration, capture, and routing of audio devices such as speakers and microphones.
- **Video Device Management (VDM)**: This module will manage the enumeration and capture of video devices such as cameras.
- **Audio and Video Stream Processing**: Includes tasks such as resampling, compression, encoding, merging, multimedia file storage, voice changing, beautification, and streaming.
- **Screen Capture On macOS**: The project provides functionality for capturing screen on macOS.
- **Screen Capture On Linux**: The project provides functionality for capturing screen on Linux.
- **Screen Capture On Android**: The project provides functionality for capturing screen on Android.
- **Screen Capture On iOS**: The project provides functionality for capturing screen on iOS.
- **Screen Capture On Linux**: The project provides functionality for capturing screen on Linux.

## How to Build

### Prerequisites
- CMake 3.x or higher
- For Linux: gcc/g++ or clang/clang++
- For Windows: Visual Studio Build Tools
- For macOS: Xcode and Command Line Tools
- For Android: Android NDK and Android Studio
- For iOS/xrOS: Xcode

### Build Steps

#### Clone the repository:
```sh
git clone --recurse-submodules https://github.com/opentraa/traa.git
cd traa
```

#### Using Build Scripts

The project provides build scripts for easy compilation:

##### On Unix-like Systems (Linux, macOS, iOS, xrOS, Android):
```sh
./scripts/build.sh -p <platform> [options]
```

Available platforms:
- `macos`: Build for macOS
- `ios`: Build for iOS
- `xros`: Build for Apple Vision Pro
- `linux`: Build for Linux
- `android`: Build for Android

Common options:
- `-t, --build-type`: Build type (Debug/Release) [default: Release]
- `-U, --unittest`: Build unit tests (ON/OFF) [default: OFF]
- `-S, --smoketest`: Build smoke tests (ON/OFF) [default: OFF]
- `-V, --verbose`: Enable verbose output
- `-v, --version`: Specify version number

Platform-specific options:
- For Android: `-A, --android-abi`: Specify ABIs (default: arm64-v8a,armeabi-v7a,x86,x86_64)
- For Linux: `-a, --arch`: Target architecture (x86_64/aarch64_clang/aarch64_gnu)

##### On Windows:
```batch
scripts\build.bat [options]
```

Available options:
- `-a, --arch`: Architecture (Win32/x64/ARM64) [default: x64]
- `-t, --build-type`: Build type (Debug/Release) [default: Release]
- `-U, --unittest`: Build unit tests (ON/OFF) [default: OFF]
- `-S, --smoketest`: Build smoke tests (ON/OFF) [default: OFF]
- `-V, --verbose`: Enable verbose output

#### Example Commands

Build for macOS:
```sh
./scripts/build.sh -p macos -t Release
```

Build for Windows (x64):
```batch
scripts\build.bat -a x64 -t Release
```

Build for Android with specific ABIs:
```sh
./scripts/build.sh -p android -A "arm64-v8a,x86_64"
```

Build for Linux with unit tests:
```sh
./scripts/build.sh -p linux -U ON
```
