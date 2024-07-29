# print current path
pwd

# if build and build/mac not exist, create it
if [ ! -d "build" ]; then
  mkdir build
fi
if [ ! -d "build/mac" ]; then
  mkdir build/mac
fi

pushd ./build/mac

# usage ./build-mac.sh [Debug|Release]
cmake "../../" \
    -G "Xcode" \
    -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake \
    -DPLATFORM="MAC_UNIVERSAL" \
    -DCMAKE_BUILD_TYPE="$1"
    
cmake --build . --config $1
popd