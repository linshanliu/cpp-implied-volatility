#include "EuropeanOption.hpp"
#include "OptionQuote.hpp"
#include "CSVReader.hpp"
#include "DataCleaner.hpp"
#include <iostream>
#include <vector>



int main()
{
    // Step 1 ==================================================================================================
    // Dividend-adjusted Black-Scholes benchmark.
    //
    // Parameters:
    // S0    = 100
    // K     = 100
    // T     = 1 year
    // r     = 5%
    // q     = 2% continuous dividend yield
    // sigma = 20%
    //
    // Expected results:
    // European Call Price ¡Ö 9.2270
    // European Put  Price ¡Ö 6.3301
    // Vega                ¡Ö 37.9012


     /*Successful reproduction of these prices provides confidence that the
     implementation of d1, d2, the option pricing formulas and vega is correct.*/

    /*EuropeanOption Option1(100.,1., true);
    std::cout << "The option price via BS formula is:" << Option1.BS_Price(100., 0.05, 0.2,0.02) << std::endl;
    std::cout << "The Vega via BS formula is:" << Option1.Vega(100., 0.05, 0.2, 0.02) << std::endl;
    std::cout <<  "The implied volatility with market price 9.2270 is:" << Option1.ImpliedVolatility(100, 0.05, 0.02, 9.2270) << std::endl;*/


   

    //Step2=================================================================================================
    /*std::vector<double> strikes = { 80,90,100,110,120 };
    std::vector<double> prices = { 24.5888,16.6994,10.4506,6.0401,3.2475 };

    for (int i = 0; i < strikes.size(); i++)
    {
        EuropeanOption option(strikes[i], 1, true);

        double iv = option.ImpliedVolatility(100, 0.05, 0, prices[i]);

        std::cout << strikes[i] << "  " << iv << std::endl;
    }*/

    

    //Step3 ================================================================================================
    /*auto quotes = CSVReader::readOptionQuotes(
        "./SPCX_options_raw.csv"
    );

    std::cout << "Loaded "
        << quotes.size()
        << " quotes.\n";

    for (size_t i = 0; i < std::min(size_t(10), quotes.size()); ++i)
    {
        const auto& q = quotes[i];

        std::cout
            << q.contractSymbol
            << "  K=" << q.strike
            << "  bid=" << q.bid
            << "  ask=" << q.ask
            << "  IV=" << q.yahooIV
            << '\n';
    }
    
    try {
        auto quotes = CSVReader::readOptionQuotes(
            R"(.\SPCX_options_raw.csv)"
        );

        std::cout << "Loaded "
            << quotes.size()
            << " quotes.\n\n";

        const std::size_t n =
            std::min<std::size_t>(10, quotes.size());

        for (std::size_t i = 0; i < n; ++i) {
            const auto& q = quotes[i];

            std::cout
                << q.contractSymbol
                << "  type=" << (q.isCall ? "Call" : "Put")
                << "  expiry=" << q.expiration
                << "  K=" << q.strike
                << "  bid=" << q.bid
                << "  ask=" << q.ask
                << "  mid=" << q.mid
                << "  volume=" << q.volume
                << "  OI=" << q.openInterest
                << "  YahooIV=" << q.yahooIV
                << '\n';
        }
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }*/


	//Step4=================================================================================================
    try {
        auto rawQuotes = CSVReader::readOptionQuotes(
            R"(.\SPCX_options_raw.csv)"
        );

        auto cleanQuotes =
            DataCleaner::clean(rawQuotes);

        std::cout
            << "Raw quotes: "
            << rawQuotes.size()
            << '\n';

        std::cout
            << "Clean quotes: "
            << cleanQuotes.size()
            << '\n';
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    
}
