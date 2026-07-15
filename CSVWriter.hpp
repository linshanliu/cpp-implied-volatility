#ifndef CSVWRITER_HPP
#define CSVWRITER_HPP


#include <string>
#include <vector>

#include "OptionQuote.hpp"

class CSVWriter
{
public:

    static void WriteOptionQuotes(
        const std::vector<OptionQuote>& quotes,
        const std::string& filename
    );

};

#endif // !CSVWRITER_HPP
