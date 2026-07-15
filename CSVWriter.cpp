#include "CSVWriter.hpp"

#include <fstream>
#include <stdexcept>

void CSVWriter::WriteOptionQuotes(
    const std::vector<OptionQuote>& quotes,
    const std::string& filename)
{
    std::ofstream file(filename);

    if (!file)
    {
        throw std::runtime_error(
            "Cannot open output file."
        );
    }

    file << "Strike,Maturity,Mid,IsCall,IV\n";

    for (const auto& quote : quotes)
    {
        file
            << quote.strike << ','
            << quote.maturity << ','
            << quote.mid << ','
            << quote.isCall << ','
            << quote.calculatedIV
            << '\n';
    }
}