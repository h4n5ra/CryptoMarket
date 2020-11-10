#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <future>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "json/json.h"
#include "Market.h"

Market::Market(std::string url) : m_url(url)
{
    this->m_key = this->get_api_key();
}

float get_price_from_json(std::string json_str)
{
    Json::Value root;
    std::stringstream sstr(json_str);

    sstr >> root;
    float price = std::stof(root["price"].asString());
    return price;
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string Market::curl(std::string url, std::string method, std::string query)
{
    std::string execute =  "curl -s -H 'X-MBX-APIKEY: "+this->m_key+"' -X "+method+" '"+url+query+"'";
    // std::cout << "Running: " << execute << std::endl;
    const char* ex = execute.c_str();
    return exec(ex);
}


std::string Market::get_timestamp()
{
    std::string url = this->m_url + "/api/v3/time";
    // std::string url = this->m_url + "/fapi/v1/time";
    std::string json = this->curl(url, "GET", "");

    bool append = false;
    std::string timestamp = "";

    for (auto c : json)
    {
        if (c == '}')
        {
            append = false;
        }
        if (append)
        {
            timestamp = timestamp + c;
        }
        if (c==':')
        {
            append = true;
        }   
    }
    return timestamp;
}

std::string Market::send_order(std::string ticker, float quantity)
{

    std::string url = this->m_url + "/api/v3/order/test?";
    // std::string url = this->m_url + "/fapi/v1/order/test?";
    std::string query = "symbol="+ticker+"&side=BUY&type=MARKET&quantity="+std::to_string(quantity)+"&recvWindow=5000&timestamp="+this->get_timestamp();
    query = query+"&signature="+this->generate_sig(query);

    auto begin = std::chrono::high_resolution_clock::now();
    std::string order = curl(url, "POST", query);
    auto end = std::chrono::high_resolution_clock::now();
    auto ping = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count()/1000.0;

    std::cout << "Latency: " << ping << "ms" << std::endl;
    
    return order;
}

float Market::get_market_price(std::string ticker, bool futures)
{   
    std::string url;
    std::string query = "?symbol="+ticker;
    if (futures)
    {
        url = this->m_url + "/fapi/v1/ticker/price";
    } 
    else 
    {
        url = this->m_url + "/api/v3/ticker/price";
    }
    
    auto begin = std::chrono::high_resolution_clock::now();
    std::string price = curl(url, "GET", query);
    auto end = std::chrono::high_resolution_clock::now();
    auto ping = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count()/1000.0;

    // std::cout << price << std::endl;
    // std::cout << "Latency: " << ping << "ms" << std::endl;
    return get_price_from_json(price);
}


void Market::get_historical(std::string ticker)
{
    std::string query = "?symbol="+ticker;
    std::string url = this->m_url + "/api/v3/aggTrades";
    std::ofstream output("/home/harvir/Code/CppBinance/output.txt");

    auto begin = std::chrono::high_resolution_clock::now();
    output << curl(url, "GET", query);
    auto end = std::chrono::high_resolution_clock::now();
    auto ping = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count()/1000.0;

    std::cout << "Latency: " << ping << "ms" << std::endl;
    output.close();
}

std::string Market::get_balance()
{
    std::string url = this->m_url + "/api/v3/account?";
    std::string query = "timestamp="+this->get_timestamp();
    query = query+"&signature="+this->generate_sig(query);
    return this->curl(url, "GET", query);
}   

void Market::ethbtc_arbitrage()
{
    float fees = 0.0f;
    float usdt = 10000.0f;
    float btc, eth = 0.0f;
    float ethusdt, ethbtc, btcusdt;

    std::future<float> ethusdt_f, ethbtc_f, btcusdt_f;

    std::cout << "Start balance: " << usdt << std::endl;
    for (int i = 1; i < 100; i++)
    {

        std::cout << "Run: " << i << std::endl;
        auto begin = std::chrono::high_resolution_clock::now();

        ethusdt_f = std::async(&Market::get_market_price, this, "ETHUSDT", false);
        ethbtc_f = std::async(&Market::get_market_price, this, "ETHBTC", false);
        btcusdt_f = std::async(&Market::get_market_price, this, "BTCUSDT", false);
        
        ethusdt = ethusdt_f.get();
        ethbtc = ethbtc_f.get();
        btcusdt = btcusdt_f.get();

        // single-threaded
        // ethusdt = this->get_market_price("ETHUSDT", false);
        // ethbtc = this->get_market_price("ETHBTC", false);
        // btcusdt = this->get_market_price("BTCUSDT", false);

        auto end = std::chrono::high_resolution_clock::now();
        auto ping = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count()/1000.0;
        std::cout << "Total latency: " << ping << "ms" << std::endl;
        
        if ((btcusdt * ethbtc) < ethusdt)
        {
            btc = (usdt/btcusdt) - (btc*fees);
            eth = (btc/ethbtc) - (eth*fees);
            usdt = (ethusdt*eth) - (usdt*fees);
            std::cout << "Arbitrage: Bought " << btc << "BTC, sold for " << eth << "ETH and sold for " << usdt << "USDT" << std::endl;
            std::cout << "\n";
        }
        else if ((ethusdt/ethbtc) > btcusdt)
        {
            eth = (usdt/ethusdt) - (eth*fees);
            btc = (eth/ethbtc) - (btc*fees);
            usdt = (btcusdt*btc) - (usdt*fees);
            std::cout << "Arbitrage: Bought " << eth << "ETH, sold for " << btc << "BTC and sold for " << usdt << "USDT" << std::endl;
            std::cout << "\n";
        }
        else
        {
            std::cout << "No arbitrage" << "\n" << std::endl;
        }
    }
    std::cout << "End balance: " << usdt << std::endl;
    std::cout << "Return: " << usdt-10000.0f << std::endl;
}


std::string Market::get_api_key()
{
    std::string line;
    std::string filepath = "/home/harvir/Code/CppBinance/api_key.txt";
    std::ifstream file(filepath);

    if (file.is_open())
    {
        std::getline(file, line);
    } 
    else
    {
        std::cout << "File could not be opened" << std::endl;
        exit(1);
    }
    
    file.close();
    return line;
}

std::string Market::get_secret_key()
{
    std::string line;
    std::string filepath = "/home/harvir/Code/CppBinance/secret_key.txt";
    std::ifstream file(filepath);

    if (file.is_open())
    {
        std::getline(file, line);
    } 
    else
    {
        std::cout << "File could not be opened" << std::endl;
        exit(1);
    }
    
    file.close();
    return line;
}


std::string Market::generate_sig(std::string query)
{
    std::string command = "echo -n '"+query+"' | openssl dgst -sha256 -hmac '"+this->get_secret_key()+"'";
    const char* ex = command.c_str();
    std::string response = exec(ex);

    bool append = false;
    std::string signature = "";

    for (auto c : response)
    {
        if (c=='\n')
        {
            append = false;
        }if (append)
        {
            signature = signature + c;
        }
        if (c==' ')
        {
            append = true;
        }
    }

    return signature;
}