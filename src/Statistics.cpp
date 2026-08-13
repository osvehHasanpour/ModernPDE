#include "Statistics.h"

#include <iostream>

void Statistics::reset()
{
    counts_.clear();
}

void Statistics::add(
    const std::string& key,
    std::size_t n)
{
    counts_[key] += n;
}

std::size_t Statistics::get(
    const std::string& key) const
{
    const auto found = counts_.find(key);

    if(found == counts_.end())
    {
        return 0;
    }

    return found->second;
}

void Statistics::print(
    const std::string& title) const
{
    std::cout
        << title
        << "\n";

    if(counts_.empty())
    {
        std::cout
            << "  (none)\n";
        return;
    }

    for(const auto& entry : counts_)
    {
        std::cout
            << "  "
            << entry.first
            << " : "
            << entry.second
            << "\n";
    }
}
