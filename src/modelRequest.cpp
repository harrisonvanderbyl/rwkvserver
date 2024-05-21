
#include "rwkv.h"
#include "tokenizer/tokenizer.hpp"
#include "sampler/sample.h"
#include "asio.hpp"

class ModelRequest
{
    public:
    asio::ip::tcp::socket* sock = nullptr;
    std::vector<std::string> stop_strings = {};
    std::vector<size_t> stop_tokens = {0};
    float* batch_index = nullptr;
    size_t* current_token = nullptr;
    float temp = 1.0;
    RWKVTokenizer* RWKVtokenizer = nullptr;
    size_t* prompt_buffer = nullptr;
    size_t prompt_buffer_size = 0;
    bool done = true;

    ModelRequest(float* batch_index, RWKVTokenizer* tokenizer, size_t* current_token)
    {
        this->batch_index = batch_index;
        this->current_token = current_token;
        this->RWKVtokenizer = tokenizer;
        
    }


    void sample(){
        if (done){
            return;
        }
        if (prompt_buffer_size > 0)
        {
            *current_token = *prompt_buffer;
            std::cout << RWKVtokenizer->decode({*current_token});
            prompt_buffer_size -= 1;
            prompt_buffer += 1;

            return;
        }

        *current_token = dart(batch_index, temp);

        for (auto stop : stop_tokens)
        {
            if (*current_token == stop)
            {
                sock->close();
                done = true;
                return;
            }
        }

        auto str = RWKVtokenizer->decode({*current_token});

        for (auto stop : stop_strings)
        {
            if (str == stop)
            {
                sock->close();
                done = true;
                return;
            }
        }

        try{
        sock->write_some(asio::buffer(str));
        }
        catch(asio::system_error e){
            std::cout << "connection closed\n";
            sock->close();
            done = true;
        }
        
    }

    void assign(asio::ip::tcp::socket* sock, std::vector<std::string> stop_strings, std::vector<size_t> stop_tokens, float temp, std::vector<size_t> prompt_buffer){
        this->sock = sock;
        this->stop_strings = stop_strings;
        this->stop_tokens.clear();
        for (auto token : stop_tokens)
        {
            this->stop_tokens.push_back(token);
        }
        this->stop_tokens.push_back(0);
        this->temp = temp;
        this->done = false;
        this->prompt_buffer = new size_t[prompt_buffer.size()];
        for (size_t i = 0; i < prompt_buffer.size(); i++)
        {
            this->prompt_buffer[i] = prompt_buffer[i];
        }
        this->prompt_buffer_size = prompt_buffer.size();
    }





};



