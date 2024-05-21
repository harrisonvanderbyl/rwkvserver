
#include "asio2/asio2.hpp"
#include "openssl/sha.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "rwkv.h"
#include "tokenizer/tokenizer.hpp"
#include "sampler/sample.h"
#include <functional>

void run(RWKV *model, Tensor logitsin, RWKVTokenizer *worldTokenizer, float temp, std::function<void(std::string output)> callback = [](std::string output)
                                                                                  { get_threadpool()->print(output); },
         std::function<void()> done = []() {})

{
    // std::cout << "Generating token " << i << std::endl;

    auto pool = get_threadpool();
    auto logs = (logitsin[0][logitsin.shape[1] - 1]);
    size_t sample = dart((float *)logs.cpu().data, temp);
    std::string output = "";
    if (sample == 0)
    {
        done();
        return;
    }
    else
    {
        output = worldTokenizer->decode({sample});
    }

    if (output == "User")
    {
        done();
        return;
    }

    callback(output);

    auto logits = (*model)({{sample}});
    pool->add_job(
        [logits, model, worldTokenizer, temp, callback, done]
        {
            run(model, logits, worldTokenizer, temp, callback, done);
        },
        0);
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

    auto model = new RWKV(file, threads);
    model->cuda();

    asio::io_service svc;
    asio::ip::tcp::acceptor acc(svc, asio::ip::tcp::endpoint(asio::ip::address(), port));

    auto sock = asio::ip::tcp::socket(svc);

    acc.async_accept(sock, [&sock, &svc, &acc, worldTokenizer, model](asio::error_code ec)
                     {
                         if (!ec)
                         {
                            // copy source file to buffer data

                             std::cout << "connection from " << sock.remote_endpoint() << "\n";

                             auto str = std::string(10000, '\0');
                             // read the request
                                asio::streambuf data;
                                asio::read_until(sock, data, "\r\n\r\n");
                                std::string body = asio::buffer_cast<const char*>(data.data());
                                // remove until the end of the header
                                body = body.substr(body.find("\r\n\r\n") + 4);

                            std::cout << body;
                            //  std::string body = str.substr(s.find("\r\n\r\n") + 4);

                             json j = json::parse(body);
                             auto prompt = j["prompt"].get<std::string>();

                             auto tokens = worldTokenizer->encode(prompt);

                             auto out = (*model)({tokens});

                            auto pool = get_threadpool();
                             auto logits = out[0][out.shape[1] - 1].cpu().data;
                             size_t coutout = 0;
                             bool done = false;

                            // write starting http response
                            std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                            sock.write_some(asio::buffer(response));

                             run(model, out, worldTokenizer, 0.5, [&coutout, &sock](std::string output)
                                 {
                                    std::cout << output << std::flush;
                                    sock.send(asio::buffer(output));
                                    coutout++; }, [&done]()
                                 { done = true; });

                             while (!done)
                             {
                                 // sleep
                                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
                             }

                             sock.close();
                         }

                         
                         // now write the whole story
                        //  asio::async_write(sock, data, [&sock, &data /*keep alive*/](asio::error_code ec, size_t transferred) {});

                          });

    svc.run();

    // while (true)
    // {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // }
}