# g++ ./server.cpp -o server -std=c++17 -lpthread -I./include/rwkv.cuh/include -I./include/asio2/include/ -I./include/asio/asio/include/ -lcrypto

# cmakelists

cmake_minimum_required(VERSION 3.10)
project(rwkv CUDA CXX)

# build with nvcc if nvcc is located in /usr/local/cuda/bin/nvcc

set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)

# set cuda arch to latest
set(CMAKE_CUDA_ARCHITECTURES 80)

message(STATUS "CUDA found")
# add_executable(server server.cpp ./include/rwkv.cuh/src/cudaops.cu)
# add sources
add_executable(server ./include/rwkv.cuh/src/cudaops.cu)
# add cpp file
target_sources(server PRIVATE server.cpp)
# target_compile_options(server PRIVATE $<$<COMPILE_LANGUAGE:CUDA>: -arch=sm_80 -O3 -std=c++17 >)
# add -march=native
target_compile_options(server PRIVATE -march=native -O3 -std=c++17 )




# set cuda arch to latest



# target_include_directories(server PRIVATE /usr/local/cuda/include)
target_link_libraries(server cuda cudart_static)





target_link_libraries(server pthread)
target_include_directories(server PRIVATE ./include/rwkv.cuh/include ./include/asio/asio/include)
