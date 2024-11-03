# print current path
pwd

# if build and build/linux not exist, create it
if [ ! -d "build" ]; then
  mkdir build
fi
if [ ! -d "build/linux" ]; then
  mkdir build/linux
fi

pushd ./build/linux

# usage ./build-linux.sh [Debug|Release]
cmake "../../" \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_BUILD_TYPE="$1"
    
cmake --build . --config $1
popd