mkdir -p build-{debug,release,profile}
cd build-debug
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Debug -GNinja
cd ../build-release
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Release -GNinja
cd ../build-profile
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=On -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-pg -GNinja
cd ..
cmake --build build-debug
cmake --build build-release
cmake --build build-profile
