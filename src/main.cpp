#include <iostream>
#include <fstream>
#include <string>

#include "Market.h"

using namespace std;

string connect_to_stream(string& uri)
{
    return "Finished";
}


int main()
{
    Market market = Market("https://api.binance.com");
    Market fmarket = Market("https://testnet.binancefuture.com");
    
    // market.ethbtc_arbitrage();
    // cout << market.get_balance() << endl;
    // cout << market.get_timestamp() << endl;
    cout << market.send_order("BTCUSDT", 1.1234) << endl;
    return 0;
}
