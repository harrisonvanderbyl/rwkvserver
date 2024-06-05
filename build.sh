# makdir ./bin if not exist
mkdir -p ./bin
export HEADERFILES=$(locate cuda_runtime.h | sed 's/\/cuda_runtime.h//')
export LIBRARYFILES=$(locate libcudart_static.a | sed 's/\/libcudart_static.a//')

# g++ -x c++ ./server.cpp -o ./bin/server -std=c++17 -lpthread -I./include/rwkv.cuh/include -I./include/asio2/include/ -I./include/asio/asio/include/ -lcrypto -march=native
nvcc ./server.cpp ./include/rwkv.cuh/src/cudaops.cu -o ./bin/server -I$HEADERFILES -std=c++17 -lpthread -I./include/rwkv.cuh/include -I./include/asio2/include/ -I./include/asio/asio/include/ -lcuda -lcudart_static -lcrypto -Xcompiler -march=native -gencode arch=compute_70,code=sm_70 -gencode arch=compute_75,code=sm_75 -gencode arch=compute_80,code=sm_80 -gencode arch=compute_86,code=sm_86 -gencode arch=compute_86,code=compute_86