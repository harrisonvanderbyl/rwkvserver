#include "openssl/sha.h"
#include "asio.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <functional>
#include "src/modelRequest.cpp"
#include "cmath"

class RwkvServer
{
public:
    asio::io_service *svc;
    asio::ip::tcp::acceptor *acc;
    asio::ip::tcp::socket *sock;
    RWKV *model;
    RWKVTokenizer *worldTokenizer;
    std::vector<ModelRequest> requests;
    size_t *inputs = nullptr;
    float *logits = nullptr;

    RwkvServer(std::string file, int threads, int port, bool device, size_t concurrency = 4)
    {
        svc = new asio::io_service();
        acc = new asio::ip::tcp::acceptor(*svc, asio::ip::tcp::endpoint(asio::ip::address(), port));
        sock = new asio::ip::tcp::socket(*svc);
        model = new RWKV(file, threads);
        if (device)
        {
            model->cuda();
        }
        worldTokenizer = new RWKVTokenizer("rwkv_vocab_v20230424.txt");

        float *logitsc = new float[concurrency * 65536];
        inputs = new size_t[concurrency];
        for (size_t i = 0; i < concurrency; i++)
        {
            inputs[i] = 0;
        }
        logits = logitsc;

        for (size_t i = 0; i < concurrency; i++)
        {
            requests.push_back(ModelRequest(logitsc + i * 65536, worldTokenizer, inputs + i));
        }
    }

    ~RwkvServer()
    {
        delete svc;
        delete acc;
        delete model;
        delete worldTokenizer;
    }

    void modelLoop()
    {
        auto pool = get_threadpool();
        auto inputbuff = std::vector<std::vector<size_t>>();
        for (size_t i = 0; i < requests.size(); i++)
        {

            inputbuff.push_back({
                requests[i].current_token[0],
            });
        }

        auto logitsout = (*model)({inputbuff});

        pool->sync();

        pool->add_job([this, logitsout, pool]

                      {
                          // copy the logits to the output buffer
                          if (logitsout.device == DEVICE::CUDA)
                          {
                              RcudaMemcpy(this->logits, logitsout.data, logitsout.data_size_in_bytes, cudaMemcpyDeviceToHost);
                          }
                          else
                          {
                              std::memcpy(this->logits, logitsout.data, logitsout.data_size_in_bytes);
                          }
                          for (size_t i = 0; i < requests.size(); i++)
                          {
                              pool->add_job([this, i]
                                            { requests[i].sample(); }, i);
                          }
                          pool->sync();

                          pool->add_job([this]
                                        { modelLoop(); }, 0); },
                      0);
    }

    void run()
    {
        auto nssock = new asio::ip::tcp::socket(*svc);
        acc->async_accept(*nssock, [this, nssock](asio::error_code ec)
                          {
                               
                              if (!ec)
                              {
                                  std::cout << "connection from " << nssock->remote_endpoint() << "\n";

                                  auto str = std::string(10000, '\0');
                                  // read the request
                                  asio::streambuf data;
                                  asio::read_until(*nssock, data, "\r\n\r\n");
                                  std::string body = asio::buffer_cast<const char *>(data.data());
                                  // remove until the end of the header
                                  body = body.substr(body.find("\r\n\r\n") + 4);

                                  std::cout << body;
                                  //  std::string body = str.substr(s.find("\r\n\r\n") + 4);

                                  json j = json::parse(body);
                                  auto prompt = j["prompt"].get<std::string>();


                                  // write starting http response
                                  std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                                  nssock->write_some(asio::buffer(response));
                                  
                                  tryAssign(nssock, prompt, j["stop"].get<std::vector<std::string>>(), j["temperature"].get<float>(), j);
                                run();
                                 
                              } });
    }

    void tryAssign(asio::ip::tcp::socket *nssock, std::string prompt, std::vector<std::string> stop, float temp, json j)

    {
        auto pool = get_threadpool();

        auto tokens = worldTokenizer->encode(prompt);

        pool->add_job([this, tokens, j, nssock]
                      {
                                                    bool assigned = false;
                                                    for (size_t i = 0; i < requests.size(); i++)
                                                    {
                                                        if (requests[i].done)
                                                        {

                                                            std::vector<std::string> stop = j["stop"].get<std::vector<std::string>>();
                                                            
                                                            model->zero_state(i);
                                                            requests[i].assign(nssock, stop, {}, j["temperature"].get<float>(), tokens);
                                                            assigned = true;
                                                            break;
                                                        }
                                                    } 
                                                    if (!assigned)
                                                    {
                                                        tryAssign(nssock, j["prompt"].get<std::string>(), j["stop"].get<std::vector<std::string>>(), j["temperature"].get<float>(), j);
                                                    } }, 0);
    }

    void stop()
    {
        svc->stop();
    }

    void start()
    {
        run();
        svc->run();
    }
};

int main(int argc, char *argv[])
{
    RWKVTokenizer *worldTokenizer = new RWKVTokenizer("rwkv_vocab_v20230424.txt");

    // file, threads, port
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg[0] == '-')
        {
            if (i + 1 < argc)
            {
                args[arg] = argv[i + 1];
                i++;
            }
            else
            {
                args[arg] = "";
            }
        }
    }

    std::string file = args.count("-model") ? args["-model"] : "model.safetensors";
    int threads = args.count("-threads") ? std::stoi(args["-threads"]) : 0;
    int port = args.count("-port") ? std::stoi(args["-port"]) : 8080;
    bool device = args.count("-cpu") ? false : true;

    RwkvServer server(file, threads, port, device);
    server.modelLoop();
    server.start();
}