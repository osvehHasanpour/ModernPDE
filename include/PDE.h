#pragma once

#include <string>
#include <unordered_map>

struct VariableInfo
{
    std::string name;

    int definitions = 0;
    int uses = 0;
    int paths = 0;
};

class PDE
{
public:

    void defineVariable(
        const std::string& name);

    void useVariable(
        const std::string& name);

    void addExecutionPath(
        const std::string& name);

    bool isDead(
        const std::string& name) const;

    bool isPartiallyDead(
        const std::string& name) const;

    double getUsageRatio(
        const std::string& name) const;

    std::string classify(
        const std::string& name) const;

    void printResults() const;

private:

    std::unordered_map<
        std::string,
        VariableInfo
    > variables;
};