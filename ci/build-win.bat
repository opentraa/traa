@REM print current path
echo %cd%

@REM if build and build/win not exist, create it 
if not exist build mkdir build
if not exist build\win mkdir build\win

pushd build\win
echo %cd%

@REM usage ./build-win.bat [Debug|Release]
cmake ^
    -DCMAKE_BUILD_TYPE=%1 ^
    "../../"
cmake --build . --config %1
popd