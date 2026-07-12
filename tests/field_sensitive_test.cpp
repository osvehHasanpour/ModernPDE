/*
 * Field-Sensitive Analysis automated regression test.
 */

#include "FieldSensitiveAnalysis.h"
#include "IR.h"

#include <iostream>
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

static Instruction makeAlloc(
    int id,
    const std::string& var,
    const std::string& site)
{
    Instruction inst;

    inst.id = id;
    inst.type = InstType::Alloc;
    inst.result = var;
    inst.uses.insert(site);

    return inst;
}

static Instruction makeCopy(
    int id,
    const std::string& lhs,
    const std::string& rhs)
{
    Instruction inst;

    inst.id = id;
    inst.type = InstType::Copy;
    inst.result = lhs;
    inst.uses.insert(rhs);

    return inst;
}

static Instruction makeFieldStore(
    int id,
    const std::string& base,
    const std::string& field,
    const std::string& value)
{
    Instruction inst;

    inst.id = id;
    inst.type = InstType::FieldStore;
    inst.result = base + "." + field;
    inst.uses.insert(value);

    return inst;
}

static Instruction makeFieldLoad(
    int id,
    const std::string& dst,
    const std::string& base,
    const std::string& field)
{
    Instruction inst;

    inst.id = id;
    inst.type = InstType::FieldLoad;
    inst.result = dst;
    inst.uses.insert(base + "." + field);

    return inst;
}

static Instruction makeEscapeCall(
    int id,
    const std::string& ptr)
{
    Instruction inst;

    inst.id = id;
    inst.type = InstType::Call;
    inst.result = "@escape";
    inst.uses.insert(ptr);

    return inst;
}

static FunctionIR buildSample()
{
    FunctionIR function;

    function.name = "fieldDemo";

    function.instructions.push_back(
        makeAlloc(0, "o1", "Point#0"));

    function.instructions.push_back(
        makeAlloc(1, "o2", "Point#1"));

    function.instructions.push_back(
        makeCopy(2, "p", "o1"));

    function.instructions.push_back(
        makeFieldStore(3, "o1", "x", "10"));

    function.instructions.push_back(
        makeFieldStore(4, "p", "y", "20"));

    function.instructions.push_back(
        makeFieldLoad(5, "r", "o2", "x"));

    function.instructions.push_back(
        makeFieldStore(6, "o2", "z", "30"));

    function.instructions.push_back(
        makeEscapeCall(7, "o1"));

    function.instructions.push_back(
        makeFieldStore(8, "o1", "w", "5"));

    function.instructions.push_back(
        makeFieldLoad(9, "t", "p", "y"));

    return function;
}

static const Instruction* findInsn(
    const FunctionIR& function,
    int id)
{
    for(const auto& inst : function.instructions)
    {
        if(inst.id == id)
        {
            return &inst;
        }
    }

    return nullptr;
}

static bool ptsContainsSite(
    const FieldSensitiveAnalysis& analysis,
    const std::string& var,
    const std::string& site)
{
    const auto& pts = analysis.pointsTo();
    const auto found = pts.find(var);

    if(found == pts.end())
    {
        return false;
    }

    for(int objectId : found->second)
    {
        for(const auto& object : analysis.objects())
        {
            if(
                object.id == objectId &&
                object.allocSite == site)
            {
                return true;
            }
        }
    }

    return false;
}

int main()
{
    std::cout
        << "================================================\n";

    std::cout
        << "  Field-Sensitive Analysis Test\n";

    std::cout
        << "================================================\n";

    FunctionIR function = buildSample();

    FieldSensitiveAnalysis analysis;

    analysis.run(function);

    CHECK(
        ptsContainsSite(analysis, "o1", "Point#0"),
        "o1 points to Point#0");

    CHECK(
        ptsContainsSite(analysis, "o2", "Point#1"),
        "o2 points to Point#1");

    CHECK(
        ptsContainsSite(analysis, "p", "Point#0"),
        "p aliases o1 (Point#0)");

    CHECK(
        !ptsContainsSite(analysis, "p", "Point#1"),
        "p does not point to Point#1");

    CHECK(
        analysis.fieldReads().size() == 2,
        "two field reads logged");

    const Instruction* insn3 =
        findInsn(function, 3);

    const Instruction* insn4 =
        findInsn(function, 4);

    const Instruction* insn6 =
        findInsn(function, 6);

    const Instruction* insn8 =
        findInsn(function, 8);

    CHECK(
        insn3 != nullptr &&
        insn3->partialDead &&
        !insn3->dead,
        "insn 3 o1.x partialDead (escaped object)");

    CHECK(
        insn4 != nullptr &&
        !insn4->dead &&
        !insn4->partialDead,
        "insn 4 p.y live (read via alias)");

    CHECK(
        insn6 != nullptr &&
        insn6->dead &&
        !insn6->partialDead,
        "insn 6 o2.z dead");

    CHECK(
        insn8 != nullptr &&
        insn8->partialDead &&
        !insn8->dead,
        "insn 8 o1.w partialDead (escaped)");

    bool distinctSites = false;

    for(const auto& object : analysis.objects())
    {
        if(object.allocSite == "Point#0")
        {
            for(const auto& other : analysis.objects())
            {
                if(other.allocSite == "Point#1")
                {
                    distinctSites = true;
                    break;
                }
            }
        }
    }

    CHECK(
        distinctSites,
        "Point#0 and Point#1 are distinct abstract objects");

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
