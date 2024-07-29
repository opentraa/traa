# print current path
pwd

# if build and build/ios not exist, create it
if [ ! -d "build" ]; then
  mkdir build
fi
if [ ! -d "build/visionos" ]; then
  mkdir build/visionos
fi

pushd ./build/visionos
pwd
cmake "../../" \
    -G "Xcode" \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake \
    -DPLATFORM="VISIONOSCOMBINED" \
    -DCMAKE_BUILD_TYPE="Release"
    
cmake --build . --config Release
popd