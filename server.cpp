
#include "asio2/asio2.hpp"
#include <iostream>
int main( int argc, char *argv[])
{
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

    asio2::http_server server;

    server.bind_recv([&](http::web_request &req, http::web_response &rep)
                     {
                         std::cout << req.path() << std::endl;
                         std::cout << req.query() << std::endl;
                     })
        .bind_connect([](auto &session_ptr)
                      { printf("client enter : %s %u %s %u\n",
                               session_ptr->remote_address().c_str(), session_ptr->remote_port(),
                               session_ptr->local_address().c_str(), session_ptr->local_port()); })
        .bind_disconnect([](auto &session_ptr)
                         { printf("client leave : %s %u %s\n",
                                  session_ptr->remote_address().c_str(), session_ptr->remote_port(),
                                  asio2::last_error_msg().c_str()); })
        .bind_start([&]()
                    { printf("start http server : %s %u %d %s\n",
                             server.listen_address().c_str(), server.listen_port(),
                             asio2::last_error_val(), asio2::last_error_msg().c_str()); })
        .bind_stop([&]()
                   { printf("stop : %d %s\n", asio2::last_error_val(), asio2::last_error_msg().c_str()); });

    server.bind<http::verb::get, http::verb::post>("/*", [](http::web_request &req, http::web_response &rep)
                                                    {
                                                        
                                                        
                                                   
                                                    // respond with hello world over the course of 2 seconds
                                                    std::string message = "Hello, world!";
                                                    rep.chunked(true);
                                                    rep.keep_alive(true);
                                                    for (auto c : message)
                                                    {
                                                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                                        // http::async_write();//rep, asio2::buffer(&c, 1));
                                                        rep.body().file();
                                                    }
                                                   },
                                                   aop_log{});



    server.bind_not_found([](http::web_request &req, http::web_response &rep)
                          { rep.fill_page(http::status::not_found); });

    server.start("0.0.0.0", port);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}