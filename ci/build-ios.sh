# print current path
pwd

# if build and build/ios not exist, create it
if [ ! -d "build" ]; then
  mkdir build
fi
if [ ! -d "build/ios" ]; then
  mkdir build/ios
fi

pushd ./build/ios
pwd
cmake "../../" \
    -G "Xcode" \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake \
    -DPLATFORM="OS64COMBINED" \
    -DCMAKE_BUILD_TYPE="Release"
    
cmake --build . --config Release
popd