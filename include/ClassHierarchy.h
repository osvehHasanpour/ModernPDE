#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class ClassHierarchy
{
public:

    void addInheritance(
        const std::string& derived,
        const std::string& base);

    std::vector<std::string>
    getChildren(
        const std::string& cls);

    std::vector<std::string>
    getAllDescendants(
        const std::string& cls);

    std::string getParent(
        const std::string& cls);

    bool hasClass(
        const std::string& cls);

    std::vector<std::string>
    getLeafClasses();

private:

    void dfs(
        const std::string& cls,
        std::unordered_set<std::string>& visited,
        std::vector<std::string>& result);

private:

    std::unordered_map<
        std::string,
        std::vector<std::string>
    > hierarchy;

    std::unordered_map<
        std::string,
        std::string
    > parentMap;

    std::unordered_set<
        std::string
    > allClasses;
};