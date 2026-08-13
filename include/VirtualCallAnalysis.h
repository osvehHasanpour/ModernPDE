#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "ClassHierarchy.h"

class VirtualCallAnalysis
{
public:

    void registerMethod(
        const std::string& className,
        const std::string& methodName);

    std::vector<std::string>
    getMethods(
        const std::string& className) const;

    bool hasMethod(
        const std::string& className,
        const std::string& methodName) const;

    std::vector<std::string>
    resolveVirtualCall(
        const std::string& baseClass,
        const std::string& methodName,
        const ClassHierarchy& CHA) const;

private:

    std::unordered_map
    <
        std::string,
        std::vector<std::string>
    > methods;
};