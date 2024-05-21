
#include "asio2/asio2.hpp"
#include "openssl/sha.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "rwkv.h"

std::string sha256(const std::string &str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
int main( int argc, char *argv[])
{
    auto a = Tensor({3});
    auto b = a.cuda();
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


    struct aop_log
    {
        bool before(http::web_request &req, http::web_response &rep)
        {
            asio2::detail::ignore_unused(rep);
            printf("aop_log before %s\n", req.method_string().data());
            return true;
        }
        bool after(std::shared_ptr<asio2::http_session> &session_ptr,
                   http::web_request &req, http::web_response &rep)
        {
            asio2::detail::ignore_unused(session_ptr, req, rep);
            printf("aop_log after\n");
            return true;
        }
    };

    struct aop_check
    {
        bool before(std::shared_ptr<asio2::http_session> &session_ptr,
                    http::web_request &req, http::web_response &rep)
        {
            asio2::detail::ignore_unused(session_ptr, req, rep);
            printf("aop_check before\n");
            return true;
        }
        bool after(http::web_request &req, http::web_response &rep)
        {
            asio2::detail::ignore_unused(req, rep);
            printf("aop_check after\n");
            return true;
        }
    };

    // create websocket client
    std::string secret = "317981cc18424575b735e5f54cd48d9a";
    std::string hashedapi_str = sha256("api-"+secret);
    std::string hashedrouter_str =  sha256("router-"+secret);
    std::string hashedagent_str =  sha256("agent-"+secret);
    std::string url = "wss://localhost";
    std::string randomid = "randomid";
    std::string path = "/agent/register/"+hashedapi_str;
    std::string wsFullUrl = path;
    // std::cout << "wsFullUrl: " << wsFullUrl << std::endl;
    auto ws = asio2::ws_client();

    auto routeSummary = "(" + wsFullUrl +  + "|" + randomid + ")";

    // ws.bind_connect([&]{

    //     std::cout << "[Agent Setup] " + routeSummary + " Connection opened";
        
    // });

    // ws.bind_recv([&](){
    //     // std::cout << message;
    // });

    // ws.start(url,4000,path);
    std::cout << "start" << std::endl;
    ws.set_host(url);

    std::cout << "set host" << std::endl;
    ws.set_port(4000);

    ws.set_upgrade_target(path);

    ws.bind_connect([](){
        std::cout << "Connected" << std::endl;
    });


    std::cout << "set port" << std::endl;

    ws.start(url,4000,path);
    ws.send("{\"type\":\"agent-setup\",\"id\":\""+randomid+"\", \"token\": \""+hashedagent_str+"\"}");

    std::cout << "Sent" << std::endl;
    

    while (true)
    {
        std::cout << "sitting" << std::endl;
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));        
    }
}