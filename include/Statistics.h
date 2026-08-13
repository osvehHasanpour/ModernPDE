#pragma once

#include <cstddef>
#include <map>
#include <string>

class Statistics
{
public:

    void reset();

    void add(
        const std::string& key,
        std::size_t n = 1);

    std::size_t get(
        const std::string& key) const;

    void print(
        const std::string& title = "Statistics") const;

private:

    std::map<std::string, std::size_t> counts_;
};
