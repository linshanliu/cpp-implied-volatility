#include "CSVReader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ctime>

using namespace std;


namespace
{
    double parseDouble(const std::string& text, double defaultValue = 0.0)
    {
        if (text.empty() || text == "N/A" || text == "nan" || text == "NaN" || text == "-") {
            return defaultValue;
        }

        try {
            return std::stod(text);
        }
        catch (const std::exception&) {
            return defaultValue;
        }
    }

    long parseLong(const std::string& text, long defaultValue = 0)
    {
        if (text.empty() || text == "N/A" || text == "nan" || text == "NaN" || text == "-") {
            return defaultValue;
        }

        try {
            return std::stol(text);
        }
        catch (const std::exception&) {
            return defaultValue;
        }
    }

    bool parseBool(const std::string& text)
    {
        return text == "True" || text == "true" || text == "1";
    }
}

// Parse "YYYY-MM-DD" to std::tm
std::tm parseDate(const std::string& dateStr) {
    std::tm tm = {};
    if (dateStr.size() == 10) {
        tm.tm_year = std::stoi(dateStr.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(dateStr.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(dateStr.substr(8, 2));
    }
    return tm;
}

// Calculate year fraction between two dates
double yearFraction(const std::tm& from, const std::tm& to) {
    std::time_t t_from = std::mktime(const_cast<std::tm*>(&from));
    std::time_t t_to = std::mktime(const_cast<std::tm*>(&to));
    double days = std::difftime(t_to, t_from) / (60 * 60 * 24);
    return days / 365.0;
}




std::vector<OptionQuote> CSVReader::readOptionQuotes(const std::string& filename)
{
    std::ifstream file(filename);


    if (!file.is_open()) {
        throw std::runtime_error("Unable to open CSV file: " + filename);
    }

    std::vector<OptionQuote> quotes;

    std::string line;

    // Skip CSV header.
    if (!std::getline(file, line)) {
        throw std::runtime_error("CSV file is empty: " + filename);
    }

    std::size_t lineNumber = 1;

    while (std::getline(file, line)) {
        ++lineNumber;

        if (line.empty()) {
            continue;
        }

        std::stringstream lineStream(line);
        std::string field;

        OptionQuote quote;

        // 1. contractSymbol
        std::getline(
            lineStream,
            quote.contractSymbol,
            ','
        );

        // 2. lastTradeDate
        std::getline(
            lineStream,
            quote.lastTradeDate,
            ','
        );

        // 3. optionType
        std::getline(lineStream, field, ',');
        quote.isCall =
            field == "C" ||
            field == "Call" ||
            field == "call";

        // 4. expiration
        std::getline(
            lineStream,
            quote.expiration,
            ','
        );

        // After reading quote.expiration
        std::tm today = {};
        std::time_t t = std::time(nullptr);
        localtime_s(&today, &t); // Windows-safe, use localtime_r on Linux

        today.tm_mday -= 1;
        mktime(&today);

        std::tm expiry = parseDate(quote.expiration);
        quote.maturity = yearFraction(today, expiry);


        // 5. strike
        std::getline(lineStream, field, ',');
        quote.strike = parseDouble(field);

        // 6. lastPrice
        std::getline(lineStream, field, ',');
        quote.lastPrice = parseDouble(field);

        // 7. bid
        std::getline(lineStream, field, ',');
        quote.bid = parseDouble(field);

        // 8. ask
        std::getline(lineStream, field, ',');
        quote.ask = parseDouble(field);

        // 9. volume
        std::getline(lineStream, field, ',');
        quote.volume = parseLong(field);

        // 10. openInterest
        std::getline(lineStream, field, ',');
        quote.openInterest = parseLong(field);

        // 11. impliedVolatility
        std::getline(lineStream, field, ',');
        quote.yahooIV = parseDouble(field);

        // 12. inTheMoney
        std::getline(lineStream, field, ',');
        quote.inTheMoney = parseBool(field);

        // Calculate mid price only when bid/ask are valid.
        if (quote.bid >= 0.0 &&
            quote.ask >= quote.bid) {
            quote.mid =
                0.5 * (quote.bid + quote.ask);
        }
        else {
            quote.mid = 0.0;
        }

        quote.calculatedIV = 0.0;
        quote.valid = true;

        if (quote.contractSymbol.empty()) {
            std::cerr
                << "Warning: skipped malformed row "
                << lineNumber
                << '\n';

            continue;
        }

        quotes.push_back(quote);
    }

    return quotes;
}