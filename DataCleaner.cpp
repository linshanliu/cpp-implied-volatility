#include "DataCleaner.hpp"

#include <cmath>

#include <cmath>
#include <iostream>

std::vector<OptionQuote> DataCleaner::clean(
    const std::vector<OptionQuote>& quotes)
{
    std::vector<OptionQuote> cleanQuotes;

    constexpr long minVolume = 10;
    constexpr long minOpenInterest = 20;

    std::size_t invalidNumber = 0;
    std::size_t invalidStrike = 0;
    std::size_t invalidMarket = 0;
    std::size_t wideSpread = 0;
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
        if (quote.strike <= 0.0) {
            ++invalidStrike;
            continue;
        }

        // Rule 3: Ask must not be below bid.
        if (quote.bid < 0.0 ||
            quote.ask < 0.0 ||
            quote.ask < quote.bid) {
            ++invalidMarket;
            continue;
        }

        // Rule 4: Require a minimum amount of trading today.
        if (quote.volume < minVolume) {
            ++lowVolume;
            continue;
        }

        // Rule 5: Require a minimum amount of outstanding positions.
        if (quote.openInterest < minOpenInterest) {
            ++lowOpenInterest;
            continue;
        }


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

    std::cout << "Invalid bid/ask: "
        << invalidMarket << '\n';

    //std::cout << "Wide spreads: "
    //    << wideSpread << '\n';

    std::cout << "Low Volume: "
        << lowVolume << '\n';

    std::cout << "Low open interest: "
        << lowOpenInterest << '\n';

    return cleanQuotes;
}