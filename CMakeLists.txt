# g++ ./server.cpp -o server -std=c++17 -lpthread -I./include/rwkv.cuh/include -I./include/asio2/include/ -I./include/asio/asio/include/ -lcrypto

# cmakelists

cmake_minimum_required(VERSION 3.10)
project(rwkv)

# build with nvcc if nvcc is located in /usr/local/cuda/bin/nvcc
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)
find_package(CUDA REQUIRED)
if(CUDA_FOUND)
    message(STATUS "CUDA found")
    cuda_add_executable(server server.cpp ./include/rwkv.cuh/src/cudaops.cu)
    
    # add -march=native
    target_compile_options(server PRIVATE -march=native -O3 -std=c++17)

    
    target_include_directories(server PRIVATE /usr/local/cuda/include)
    target_link_directories(server PRIVATE /usr/local/cuda/lib64)
    target_link_libraries(server cuda cudart_static)
    


else()
    message(STATUS "CUDA not found")
    add_executable(server server.cpp)
endif()


target_link_libraries(server pthread crypto)
target_include_directories(server PRIVATE ./include/rwkv.cuh/include ./include/asio2/include/ ./include/asio/asio/include/)
