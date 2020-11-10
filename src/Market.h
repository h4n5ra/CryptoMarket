#ifndef __MARKET_H_INCLUDED__
#define __MARKET_H_INCLUDED__

#include <string>

class Market
{
  public:
    Market(std::string url);
    void get_historical(std::string ticker);
    std::string get_timestamp();
    std::string get_balance();
    std::string send_order(std::string ticker, float quantity);
    float get_market_price(std::string ticker, bool futures);

    void ethbtc_arbitrage();

  private:
    std::string m_url;
    std::string m_key;
    std::string get_api_key();  
    std::string get_secret_key();
    std::string generate_sig(std::string query);
    std::string curl(std::string url, std::string method, std::string ticker);
};

#endif