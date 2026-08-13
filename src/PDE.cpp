#include "PDE.h"

#include <iostream>
#include <iomanip>

void PDE::defineVariable(
    const std::string& name)
{
    variables[name].name = name;

    variables[name].definitions++;
}

void PDE::useVariable(
    const std::string& name)
{
    variables[name].uses++;
}

void PDE::addExecutionPath(
    const std::string& name)
{
    variables[name].paths++;
}

bool PDE::isDead(
    const std::string& name) const
{
    const auto found = variables.find(name);

    if(found == variables.end())
    {
        return false;
    }

    return
        found->second.definitions > 0 &&
        found->second.uses == 0;
}

bool PDE::isPartiallyDead(
    const std::string& name) const
{
    const auto found = variables.find(name);

    if(found == variables.end() ||
       found->second.paths == 0)
    {
        return false;
    }

    return
        found->second.uses > 0 &&
        found->second.uses <
        found->second.paths;
}

double PDE::getUsageRatio(
    const std::string& name) const
{
    const auto found = variables.find(name);

    if(found == variables.end() ||
       found->second.paths == 0)
    {
        return 0.0;
    }

    return
        static_cast<double>(
            found->second.uses)
        /
        static_cast<double>(
            found->second.paths);
}

std::string PDE::classify(
    const std::string& name) const
{
    if(isDead(name))
    {
        return "DEAD";
    }

    double ratio =
        getUsageRatio(name);

    if(ratio < 0.25)
    {
        return "MOSTLY DEAD";
    }

    if(ratio < 0.75)
    {
        return "PARTIALLY DEAD";
    }

    if(ratio < 1.0)
    {
        return "MOSTLY LIVE";
    }

    return "LIVE";
}

void PDE::printResults() const
{
    std::cout
    << "\n====================================\n";

    std::cout
    << "PATH-AWARE PDE RESULTS\n";

    std::cout
    << "====================================\n";

    for(const auto& pair : variables)
    {
        const auto& var =
            pair.second;

        std::cout
        << "\nVariable : "
        << var.name
        << "\n";

        std::cout
        << "Definitions : "
        << var.definitions
        << "\n";

        std::cout
        << "Uses : "
        << var.uses
        << "\n";

        std::cout
        << "Paths : "
        << var.paths
        << "\n";

        std::cout
        << "Usage Ratio : "
        << std::fixed
        << std::setprecision(2)
        << getUsageRatio(
            var.name)
        << "\n";

        std::cout
        << "Classification : "
        << classify(
            var.name)
        << "\n";
    }
}