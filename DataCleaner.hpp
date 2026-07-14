#ifndef DATACLEANER_HPP
#define DATACLEANER_HPP

#include "OptionQuote.hpp"

#include <vector>

class DataCleaner
{
public:
    static std::vector<OptionQuote> clean(const std::vector<OptionQuote>& quotes);
};

#endif // !DATACLEANER_HPP
