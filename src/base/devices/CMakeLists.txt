cmake_minimum_required(VERSION 3.10)

if(NOT ANDROID AND NOT IOS AND NOT VISIONOS)
    add_subdirectory(screen)
endif()
