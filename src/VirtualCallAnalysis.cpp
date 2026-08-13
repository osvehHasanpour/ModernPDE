#include "VirtualCallAnalysis.h"

void VirtualCallAnalysis::registerMethod(
    const std::string& className,
    const std::string& methodName)
{
    methods[className]
        .push_back(methodName);
}

std::vector<std::string>
VirtualCallAnalysis::getMethods(
    const std::string& className) const
{
    const auto found = methods.find(className);

    if(found == methods.end())
    {
        return {};
    }

    return found->second;
}

bool VirtualCallAnalysis::hasMethod(
    const std::string& className,
    const std::string& methodName) const
{
    const auto found = methods.find(className);

    if(found == methods.end())
    {
        return false;
    }

    for(const auto& method : found->second)
    {
        if(method == methodName)
        {
            return true;
        }
    }

    return false;
}

std::vector<std::string>
VirtualCallAnalysis::resolveVirtualCall(
    const std::string& baseClass,
    const std::string& methodName,
    const ClassHierarchy& CHA) const
{
    std::vector<std::string> targets;

    auto descendants =
        CHA.getAllDescendants(baseClass);

    for(const auto& cls :
        descendants)
    {
        if(hasMethod(
            cls,
            methodName))
        {
            targets.push_back(
                cls +
                "::" +
                methodName);
        }
    }

    return targets;
}