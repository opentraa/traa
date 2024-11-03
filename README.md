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

## Implemented Features

- **ASIO-based Asynchronous Threading Model**: The project includes a task timer that executes tasks repeatedly at specified intervals using `asio::io_context`. This model ensures efficient task scheduling and execution.
- **Screen Source Enumeration**: The project provides functionality for enumerating screen sources on Windows and macOS, retrieving screen source information such as icon size and thumbnail size.

## Unimplemented Features

- **Audio Device Management (ADM)**: This module will handle the enumeration, capture, and routing of audio devices such as speakers and microphones.
- **Video Device Management (VDM)**: This module will manage the enumeration and capture of video devices such as cameras.
- **Audio and Video Stream Processing**: Includes tasks such as resampling, compression, encoding, merging, multimedia file storage, voice changing, beautification, and streaming.

## Contribution

We welcome contributions from the community. Feel free to open issues or submit pull requests to help improve `traa`.

## Motivation

If you find this project useful, a star on GitHub would be greatly appreciated. Your support motivates us to keep improving and adding new features.

## How to Build

### Prerequisites
- Ensure you have CMake installed on your system.
- For Linux, you will need `gcc` and `g++`.
- For Windows, you will need the Visual Studio Build Tools.
- For macOS, you will need `clang` and `clang++`.

### Build Steps

#### Clone the repository:
1. Clone the repository:
    ```sh
    git clone --recurse-submodules https://github.com/opentraa/traa.git
    cd traa
    ```

#### Linux
1. Create a build directory and configure CMake:
    ```sh
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    ```
2. Build the project:
    ```sh
    cmake --build .
    ```
3. Run unit tests:
    ```sh
    cd tests/unit_test
    ASAN_OPTIONS=detect_leaks=1 ctest --output-on-failure --timeout 300
    ```
4. Run smoke tests:
    ```sh
    cd tests/smoke_test
    ctest --output-on-failure --timeout 300
    ```

#### Windows
1. Create a build directory and configure CMake:
    ```sh
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    ```
2. Build the project:
    ```sh
    cmake --build . --config Release
    ```
3. Run unit tests:
    ```sh
    cd tests/unit_test
    ctest --build-config Release --output-on-failure --timeout 300
    ```
4. Run smoke tests:
    ```sh
    cd tests/smoke_test
    ctest --build-config Release --output-on-failure --timeout 300
    ```

#### macOS
1. Create a build directory and configure CMake:
    ```sh
    mkdir build && cd build
    cmake -G Xcode -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/ios.toolchain.cmake -DPLATFORM=MAC_UNIVERSAL ..
    ```
2. Build the project:
    ```sh
    cmake --build . --config Release
    ```
3. Run unit tests:
    ```sh
    cd tests/unit_test
    ctest --build-config Release --output-on-failure --timeout 300
    ```
4. Run smoke tests:
    ```sh
    cd tests/smoke_test
    ctest --build-config Release --output-on-failure --timeout 300
    ```
