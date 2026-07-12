/*
 * Class hierarchy and virtual-call resolution tests.
 */

#include "ClassHierarchy.h"
#include "VirtualCallAnalysis.h"

#include <iostream>
#include <set>
#include <string>

static int g_total  = 0;
static int g_passes = 0;
static int g_fails  = 0;

#define CHECK(cond, msg) do { \
    g_total++; \
    if(cond) { g_passes++; } \
    else { \
        g_fails++; \
        std::cout << "  FAIL  " << (msg) << "\n"; \
    } \
} while(0)

static bool setContainsAll(
    const std::vector<std::string>& values,
    const std::initializer_list<const char*> expected)
{
    std::set<std::string> actual(
        values.begin(),
        values.end());

    for(const char* item : expected)
    {
        if(actual.count(item) == 0)
        {
            return false;
        }
    }

    return true;
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  CHA + Virtual Call Test\n";

    std::cout
        << "================================================\n";

    ClassHierarchy hierarchy;

    hierarchy.addInheritance("Character", "Entity");
    hierarchy.addInheritance("Player", "Character");
    hierarchy.addInheritance("NPC", "Character");
    hierarchy.addInheritance("Warrior", "Player");
    hierarchy.addInheritance("Mage", "Player");
    hierarchy.addInheritance("Archer", "Player");
    hierarchy.addInheritance("Merchant", "NPC");
    hierarchy.addInheritance("QuestGiver", "NPC");
    hierarchy.addInheritance("Weapon", "Entity");
    hierarchy.addInheritance("Potion", "Entity");
    hierarchy.addInheritance("Sword", "Weapon");
    hierarchy.addInheritance("Bow", "Weapon");
    hierarchy.addInheritance("HealthPotion", "Potion");
    hierarchy.addInheritance("ManaPotion", "Potion");

    CHECK(
        hierarchy.getParent("Warrior") == "Player",
        "Warrior parent is Player");

    CHECK(
        hierarchy.getParent("Merchant") == "NPC",
        "Merchant parent is NPC");

    auto descendants =
        hierarchy.getAllDescendants("Character");

    CHECK(
        descendants.size() >= 6,
        "Character has multiple descendants");

    auto leaves =
        hierarchy.getLeafClasses();

    CHECK(
        leaves.size() >= 6,
        "multiple leaf classes exist");

    VirtualCallAnalysis virtualCalls;

    virtualCalls.registerMethod("Warrior", "attack");
    virtualCalls.registerMethod("Mage", "attack");
    virtualCalls.registerMethod("Archer", "attack");
    virtualCalls.registerMethod("Merchant", "talk");
    virtualCalls.registerMethod("QuestGiver", "talk");
    virtualCalls.registerMethod("Sword", "use");
    virtualCalls.registerMethod("Bow", "use");
    virtualCalls.registerMethod("HealthPotion", "use");
    virtualCalls.registerMethod("ManaPotion", "use");

    auto attackTargets =
        virtualCalls.resolveVirtualCall(
            "Player",
            "attack",
            hierarchy);

    CHECK(
        setContainsAll(
            attackTargets,
            {
                "Warrior::attack",
                "Mage::attack",
                "Archer::attack"
            }),
        "Player::attack resolves to concrete classes");

    auto talkTargets =
        virtualCalls.resolveVirtualCall(
            "NPC",
            "talk",
            hierarchy);

    CHECK(
        setContainsAll(
            talkTargets,
            {
                "Merchant::talk",
                "QuestGiver::talk"
            }),
        "NPC::talk resolves correctly");

    auto useTargets =
        virtualCalls.resolveVirtualCall(
            "Entity",
            "use",
            hierarchy);

    CHECK(
        setContainsAll(
            useTargets,
            {
                "Sword::use",
                "Bow::use",
                "HealthPotion::use",
                "ManaPotion::use"
            }),
        "Entity::use resolves to weapon/potion uses");

    std::cout
        << "\n================================================\n";

    std::cout
        << "  Total : "
        << g_total
        << "  Pass  : "
        << g_passes
        << "  Fail  : "
        << g_fails
        << "\n";

    if(g_fails == 0)
    {
        std::cout
            << "  RESULT : ALL TESTS PASSED\n";
    }
    else
    {
        std::cout
            << "  RESULT : "
            << g_fails
            << " TEST(S) FAILED\n";
    }

    std::cout
        << "================================================\n";

    return g_fails != 0 ? 1 : 0;
}
