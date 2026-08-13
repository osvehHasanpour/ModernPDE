#include "ClassHierarchy.h"

void ClassHierarchy::addInheritance(
    const std::string& derived,
    const std::string& base)
{
    hierarchy[base].push_back(
        derived
    );

    parentMap[derived] = base;

    allClasses.insert(base);
    allClasses.insert(derived);
}

std::vector<std::string>
ClassHierarchy::getChildren(
    const std::string& cls) const
{
    const auto found = hierarchy.find(cls);

    if(found == hierarchy.end())
    {
        return {};
    }

    return found->second;
}

void ClassHierarchy::dfs(
    const std::string& cls,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& result) const
{
    if (visited.count(cls))
        return;

    visited.insert(cls);

    const auto found = hierarchy.find(cls);

    if(found == hierarchy.end())
    {
        return;
    }

    for (const auto& child : found->second)
    {
        result.push_back(child);

        dfs(
            child,
            visited,
            result
        );
    }
}

std::vector<std::string>
ClassHierarchy::getAllDescendants(
    const std::string& cls) const
{
    std::unordered_set<std::string> visited;

    std::vector<std::string> result;

    dfs(
        cls,
        visited,
        result
    );

    return result;
}

std::string ClassHierarchy::getParent(
    const std::string& cls) const
{
    const auto found = parentMap.find(cls);

    if(found != parentMap.end())
        return found->second;

    return "";
}

bool ClassHierarchy::hasClass(
    const std::string& cls) const
{
    return allClasses.count(cls);
}

std::vector<std::string>
ClassHierarchy::getLeafClasses() const
{
    std::vector<std::string> leaves;

    for(const auto& cls : allClasses)
    {
        const auto found = hierarchy.find(cls);

        if(found == hierarchy.end() || found->second.empty())
        {
            leaves.push_back(cls);
        }
    }

    return leaves;
}