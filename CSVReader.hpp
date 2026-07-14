#ifndef CSVREADER_HPP
#define CSVREADER_HPP

#include "OptionQuote.hpp"

#include <string>
#include <vector>

class CSVReader
{
public:
    static std::vector<OptionQuote> readOptionQuotes(
        const std::string& filename
    );
};

#endif //!CSVREADER_HPP
