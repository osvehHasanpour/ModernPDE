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
    const std::string& cls)
{
    return hierarchy[cls];
}

void ClassHierarchy::dfs(
    const std::string& cls,
    std::unordered_set<std::string>& visited,
    std::vector<std::string>& result)
{
    if (visited.count(cls))
        return;

    visited.insert(cls);

    for (auto& child : hierarchy[cls])
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
    const std::string& cls)
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
    const std::string& cls)
{
    if(parentMap.count(cls))
        return parentMap[cls];

    return "";
}

bool ClassHierarchy::hasClass(
    const std::string& cls)
{
    return allClasses.count(cls);
}

std::vector<std::string>
ClassHierarchy::getLeafClasses()
{
    std::vector<std::string> leaves;

    for(auto& cls : allClasses)
    {
        if(hierarchy[cls].empty())
        {
            leaves.push_back(cls);
        }
    }

    return leaves;
}