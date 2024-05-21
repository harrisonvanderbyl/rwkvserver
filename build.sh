# makdir ./bin if not exist
mkdir -p ./bin

g++ -x c++ ./server.cpp -o ./bin/server -std=c++17 -lpthread -I./include/rwkv.cuh/include -I./include/asio2/include/ -I./include/asio/asio/include/ -lcrypto -march=native