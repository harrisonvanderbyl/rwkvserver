# create build directory and run cmake

# create build directory
mkdir -p build

# run cmake
cd build
cmake ..
make
cd ..

# copy the binary to bin
# makdir ./bin if not exist
mkdir -p ./bin
cp ./build/server ./bin/server