#ifndef OPTIONQUOTE_HPP
#define OPTIONQUOTE_HPP

#include <string>

struct OptionQuote
{
    std::string contractSymbol;
    std::string lastTradeDate;
    std::string expiration;

    bool isCall = true;
    bool inTheMoney = false;

    double strike = 0.0;
    double maturity = 0.0;

    double lastPrice = 0.0;
    double bid = 0.0;
    double ask = 0.0;
    double mid = 0.0;

    long volume = 0;
    long openInterest = 0;

    double yahooIV = 0.0;
    double calculatedIV = 0.0;

    bool valid = true;
};

#endif // !OPTIONQUOTE_HPP
