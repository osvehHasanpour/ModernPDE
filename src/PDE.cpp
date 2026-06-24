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
    const std::string& name)
{
    return
        variables[name].definitions > 0 &&
        variables[name].uses == 0;
}

bool PDE::isPartiallyDead(
    const std::string& name)
{
    if(
        variables[name].paths == 0)
    {
        return false;
    }

    return
        variables[name].uses > 0 &&
        variables[name].uses <
        variables[name].paths;
}

double PDE::getUsageRatio(
    const std::string& name)
{
    if(
        variables[name].paths == 0)
    {
        return 0.0;
    }

    return
        static_cast<double>(
            variables[name].uses)
        /
        static_cast<double>(
            variables[name].paths);
}

std::string PDE::classify(
    const std::string& name)
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

void PDE::printResults()
{
    std::cout
    << "\n====================================\n";

    std::cout
    << "PATH-AWARE PDE RESULTS\n";

    std::cout
    << "====================================\n";

    for(auto& pair : variables)
    {
        auto& var =
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