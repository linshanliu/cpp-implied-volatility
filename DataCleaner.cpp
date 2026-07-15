#include "DataCleaner.hpp"

#include <cmath>


#include <iostream>

std::vector<OptionQuote> DataCleaner::clean(
    const std::vector<OptionQuote>& quotes, double S0)
{
    std::vector<OptionQuote> cleanQuotes;

    constexpr long minVolume = 10;
    constexpr long minOpenInterest = 20;

    std::size_t invalidNumber = 0;
    std::size_t invalidStrike = 0;
    std::size_t invalidMarket = 0;
    //std::size_t wideSpread = 0;
    std::size_t lowVolume = 0;
    std::size_t lowOpenInterest = 0;

    for (OptionQuote quote : quotes) {



        // Rule 1: Required numerical fields must be finite.
        if (!std::isfinite(quote.strike) ||
            !std::isfinite(quote.bid) ||
            !std::isfinite(quote.ask)) {
            ++invalidNumber;
            continue;
        }


        // Rule 2: Strike must be positive.
        if (quote.strike < 0.5 * S0 || quote.strike > 2 * S0)
		{
            ++invalidStrike;
            continue;
        }

		// Rule 3: Maturity must be at least 3 days (3/365 years).
        if (quote.maturity < 7.0 / 365.0)
            continue;


        // Rule 4: Ask must not be below bid. (end of day data, bid = ask = 0)
        /*if (quote.bid < 0.0 ||
            quote.ask < 0.0 ||
            quote.ask < quote.bid) {
            ++invalidMarket;
            continue;
        }*/

        if (quote.bid <= 0.0 && quote.ask <= 0.0) {
            if (quote.lastPrice > 0.0) {
                quote.mid = quote.lastPrice;
            }
            else {
                ++invalidMarket;
                continue;
            }
        }
        else {
            if (quote.bid < 0.0 || quote.ask < 0.0 || quote.ask < quote.bid) {
                ++invalidMarket;
                continue;
            }
            quote.mid = 0.5 * (quote.bid + quote.ask);
        }


        // Rule 5: Mid price must be at least 0.1.
        if (quote.mid < 3)
            continue;

        // 4. mid 大于等于内在价值
        double intrinsic = quote.isCall ? std::max(0.0, S0 - quote.strike) : std::max(0.0, quote.strike - S0);
        if (quote.mid < intrinsic)
            continue;

        // Rule 6: Require a minimum amount of trading today.
        if (quote.volume < minVolume) {
            ++lowVolume;
            continue;
        }

        // Rule 7: Require a minimum amount of outstanding positions.
        if (quote.openInterest < minOpenInterest) {
            ++lowOpenInterest;
            continue;
        }

		// Rule 8: Require a reasonable bid/ask spread.(end of day data, bid = ask = 0)
        //quote.mid = 0.5 * (quote.bid + quote.ask);

        //const double relativeSpread =
        //    (quote.ask - quote.bid) / quote.mid;

        //if (relativeSpread > 0.50) {
        //    ++wideSpread;
        //    continue;
        //}



        cleanQuotes.push_back(quote);
    }

    std::cout << "Invalid numerical values: "
        << invalidNumber << '\n';

    std::cout << "Invalid strikes: "
        << invalidStrike << '\n';

    //std::cout << "Invalid bid/ask: "
    //    << invalidMarket << '\n';

    //std::cout << "Wide spreads: "
    //    << wideSpread << '\n';

    std::cout << "Low Volume: "
        << lowVolume << '\n';

    std::cout << "Low open interest: "
        << lowOpenInterest << '\n';

    std::cout << "Clean option quotes: " << cleanQuotes.size() << std::endl;

   
    return cleanQuotes;
}