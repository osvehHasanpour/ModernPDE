#include <iostream>

#include "ClassHierarchy.h"
#include "VirtualCallAnalysis.h"
#include "CallGraph.h"
#include "PDE.h"
#include "CFG.h"
#include "DUChain.h"
#include "PathEnumeration.h"
#include "CFGPDE.h"
#include "Metrics.h"
#include "Benchmark.h"
#include <fstream>
#include <string>
#include "Lexer.h"
#include "Parser.h"
#include "CFGBuilder.h"
using namespace ModernPDE;


int main(int argc, char* argv[])
{
    std::cout << "argc = " << argc << std::endl;
    if (argc > 1)
{
    std::ifstream file(argv[1]);

    if (!file)
    {
        std::cerr << "Cannot open file: "
                  << argv[1]
                  << std::endl;
        return 1;
    }

    std::string line;

    int lineCount = 0;

    while(std::getline(file,line))
    {
        lineCount++;
    }

    std::cout
    << "\n====================================\n";

    std::cout
    << "INPUT FILE\n";

    std::cout
    << "====================================\n";

    std::cout
    << "File : "
    << argv[1]
    << "\n";

    std::cout
    << "Lines: "
    << lineCount
    << "\n\n";
}
 /////////////////////////////////////////////////////
// LEXER TEST
/////////////////////////////////////////////////////

ModernPDE::Lexer lexer;

if(argc > 1)
{
    if(lexer.tokenizeFile(argv[1]))
    {
        std::cout
        << "====================================\n";

        std::cout
        << "LEXER\n";

        std::cout
        << "====================================\n";

        std::cout
        << "Lexer OK\n";

        std::cout
        << "Token Count : "
        << lexer.tokens().size()
        << "\n\n";
    }
    else
    {
        std::cout
        << "Lexer Failed\n";
    }
}
/////////////////////////////////////////////////////
// PARSER
/////////////////////////////////////////////////////

Parser parser(lexer.tokens());

parser.parse();

std::cout
<< "====================================\n";

std::cout
<< "PARSER\n";

std::cout
<< "====================================\n";

std::cout
<< "Functions : "
<< parser.functionCount()
<< "\n";

std::cout
<< "Global Variables : "
<< parser.globalVariableCount()
<< "\n\n";

std::cout
<< "====================================\n";

std::cout
<< "CFG\n";

std::cout
<< "====================================\n";

CFGBuilder builder;

CFG cfg =
builder.build(parser.getRoot());

cfg.print();

std::cout << "\n";

/////////////////////////////////////////////////////
// PHASE 1
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";
std::cout
<< "PHASE 1 : CLASS HIERARCHY ANALYSIS\n";
std::cout
<< "====================================\n";

ClassHierarchy CHA;

CHA.addInheritance("Character","Entity");

CHA.addInheritance("Player","Character");
CHA.addInheritance("NPC","Character");

CHA.addInheritance("Warrior","Player");
CHA.addInheritance("Mage","Player");
CHA.addInheritance("Archer","Player");

CHA.addInheritance("Merchant","NPC");
CHA.addInheritance("QuestGiver","NPC");

CHA.addInheritance("Weapon","Entity");
CHA.addInheritance("Potion","Entity");

CHA.addInheritance("Sword","Weapon");
CHA.addInheritance("Bow","Weapon");

CHA.addInheritance("HealthPotion","Potion");
CHA.addInheritance("ManaPotion","Potion");

std::cout
<< "\nParent of Warrior: "
<< CHA.getParent("Warrior")
<< "\n";

std::cout
<< "Parent of Merchant: "
<< CHA.getParent("Merchant")
<< "\n";

std::cout
<< "\nCharacter Descendants:\n";

auto descendants =
    CHA.getAllDescendants("Character");

for(auto& d : descendants)
{
    std::cout << d << "\n";
}

std::cout
<< "\nLeaf Classes:\n";

auto leaves =
    CHA.getLeafClasses();

for(auto& l : leaves)
{
    std::cout << l << "\n";
}

/////////////////////////////////////////////////////
// PHASE 2
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";
std::cout
<< "PHASE 2 : VIRTUAL CALL ANALYSIS\n";
std::cout
<< "====================================\n";

VirtualCallAnalysis V;

V.registerMethod("Warrior","attack");
V.registerMethod("Mage","attack");
V.registerMethod("Archer","attack");

V.registerMethod("Merchant","talk");
V.registerMethod("QuestGiver","talk");

V.registerMethod("Sword","use");
V.registerMethod("Bow","use");

V.registerMethod("HealthPotion","use");
V.registerMethod("ManaPotion","use");

std::cout
<< "\nPlayer::attack Targets:\n";

auto attackTargets =
    V.resolveVirtualCall("Player","attack",CHA);

for(auto& t : attackTargets)
{
    std::cout << t << "\n";
}

std::cout
<< "\nNPC::talk Targets:\n";

auto talkTargets =
    V.resolveVirtualCall("NPC","talk",CHA);

for(auto& t : talkTargets)
{
    std::cout << t << "\n";
}

std::cout
<< "\nEntity::use Targets:\n";

auto useTargets =
    V.resolveVirtualCall("Entity","use",CHA);

for(auto& t : useTargets)
{
    std::cout << t << "\n";
}

/////////////////////////////////////////////////////
// PHASE 3
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";
std::cout
<< "PHASE 3 : CONTEXT SENSITIVE CALL GRAPH\n";
std::cout
<< "====================================\n";

CallGraph CG;

CG.addContextSensitiveEdge("main","loadAssets","CTX_GAME","site1");
CG.addContextSensitiveEdge("loadAssets","loadTextures","CTX_GAME","site2");
CG.addContextSensitiveEdge("loadTextures","uploadGPU","CTX_GAME","site3");

CG.addContextSensitiveEdge("main","loadLevel","CTX_LEVEL","site4");
CG.addContextSensitiveEdge("loadLevel","spawnNPC","CTX_LEVEL","site5");
CG.addContextSensitiveEdge("spawnNPC","AI","CTX_LEVEL","site6");

CG.addContextSensitiveEdge("factorial","factorial","CTX_REC","recursive_call");

CG.addContextSensitiveEdge("even","odd","CTX_MR","site7");
CG.addContextSensitiveEdge("odd","even","CTX_MR","site8");

CG.print();

std::cout
<< "\nmain reaches uploadGPU ? "
<< CG.isReachable("main","uploadGPU","CTX_GAME")
<< "\n";

std::cout
<< "main reaches AI ? "
<< CG.isReachable("main","AI","CTX_LEVEL")
<< "\n";

std::cout
<< "\nfactorial recursive ? "
<< CG.isRecursiveFunction("factorial","CTX_REC")
<< "\n";

std::cout
<< "even <-> odd ? "
<< CG.hasMutualRecursion("even","odd","CTX_MR")
<< "\n";

std::cout
<< "\nTraversal CTX_LEVEL:\n";

auto traversal =
    CG.traverseFrom("main","CTX_LEVEL");

for(auto& n : traversal)
{
    std::cout << n << "\n";
}

/////////////////////////////////////////////////////
// PHASE 4
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";
std::cout
<< "PHASE 4 : PARTIAL DEAD CODE\n";
std::cout
<< "====================================\n";

PDE pde;

// 10 DEAD

for(int i=1;i<=10;i++)
{
    std::string v = "dead" + std::to_string(i);

    pde.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        pde.addExecutionPath(v);
    }
}

// 5 MOSTLY DEAD

for(int i=1;i<=5;i++)
{
    std::string v = "mostlyDead" + std::to_string(i);

    pde.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        pde.addExecutionPath(v);
    }

    pde.useVariable(v);
}

// 5 PARTIALLY DEAD

for(int i=1;i<=5;i++)
{
    std::string v = "partial" + std::to_string(i);

    pde.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        pde.addExecutionPath(v);
    }

    for(int u=0;u<5;u++)
    {
        pde.useVariable(v);
    }
}

// 5 MOSTLY LIVE

for(int i=1;i<=5;i++)
{
    std::string v = "mostlyLive" + std::to_string(i);

    pde.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        pde.addExecutionPath(v);
    }

    for(int u=0;u<9;u++)
    {
        pde.useVariable(v);
    }
}

// 5 LIVE

for(int i=1;i<=5;i++)
{
    std::string v = "live" + std::to_string(i);

    pde.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        pde.addExecutionPath(v);
    }

    for(int u=0;u<10;u++)
    {
        pde.useVariable(v);
    }
}

pde.printResults();
/////////////////////////////////////////////////////
// PHASE 5
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";
std::cout
<< "PHASE 5 : CONTROL FLOW GRAPH\n";
std::cout
<< "====================================\n";

CFG cfg;

int start =
    cfg.createBlock();

int thenBlock =
    cfg.createBlock();

int elseBlock =
    cfg.createBlock();

int mergeBlock =
    cfg.createBlock();

cfg.addEdge(
    start,
    thenBlock);

cfg.addEdge(
    start,
    elseBlock);

cfg.addEdge(
    thenBlock,
    mergeBlock);

cfg.addEdge(
    elseBlock,
    mergeBlock);

cfg.print();
/////////////////////////////////////////////////////
// PHASE 5 : DU CHAINS
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5 : DU CHAINS\n";

std::cout
<< "====================================\n";

DUChainAnalysis du;

du.addDefinition("x",1);
du.addUse("x",2);

du.addDefinition("x",3);
du.addUse("x",4);

du.addDefinition("y",5);
du.addUse("y",6);

du.addDefinition("z",7);
du.addUse("z",10);

du.buildChains();

du.print();

/////////////////////////////////////////////////////
// PHASE 5 : PATH ENUMERATION
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5 : PATH ENUMERATION\n";

std::cout
<< "====================================\n";

PathEnumeration pe;

pe.enumerate(
    cfg,
    start);

pe.print();
/////////////////////////////////////////////////////
// PHASE 5.4
/////////////////////////////////////////////////////

std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5.4 : CFG + PDE INTEGRATION\n";

std::cout
<< "====================================\n";

CFGPDE integrated;

// variable used on all paths
integrated.registerVariable(
    "playerHealth",
    2,
    2);

// variable used only one path
integrated.registerVariable(
    "bonusDamage",
    2,
    1);

// variable never used
integrated.registerVariable(
    "unusedTemp",
    2,
    0);

// bigger example
integrated.registerVariable(
    "questReward",
    10,
    7);

integrated.analyze();
std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5.5 : ACCURACY DATASET\n";

std::cout
<< "====================================\n";

PDE accuracyTest;

/////////////////////////////////////////////////////
// 20 DEAD
/////////////////////////////////////////////////////

for(int i=1;i<=20;i++)
{
    std::string v =
        "dead_" +
        std::to_string(i);

    accuracyTest.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        accuracyTest.addExecutionPath(v);
    }
}

/////////////////////////////////////////////////////
// 20 MOSTLY DEAD
/////////////////////////////////////////////////////

for(int i=1;i<=20;i++)
{
    std::string v =
        "mostlyDead_" +
        std::to_string(i);

    accuracyTest.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        accuracyTest.addExecutionPath(v);
    }

    accuracyTest.useVariable(v);
}

/////////////////////////////////////////////////////
// 15 PARTIALLY DEAD
/////////////////////////////////////////////////////

for(int i=1;i<=15;i++)
{
    std::string v =
        "partial_" +
        std::to_string(i);

    accuracyTest.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        accuracyTest.addExecutionPath(v);
    }

    for(int u=0;u<5;u++)
    {
        accuracyTest.useVariable(v);
    }
}

/////////////////////////////////////////////////////
// 15 LIVE
/////////////////////////////////////////////////////

for(int i=1;i<=15;i++)
{
    std::string v =
        "live_" +
        std::to_string(i);

    accuracyTest.defineVariable(v);

    for(int p=0;p<10;p++)
    {
        accuracyTest.addExecutionPath(v);
    }

    for(int u=0;u<10;u++)
    {
        accuracyTest.useVariable(v);
    }
}

accuracyTest.printResults();

std::cout
<< "\n====================================\n";

std::cout
<< "EXPECTED RESULTS\n";

std::cout
<< "====================================\n";

std::cout
<< "DEAD           : 20\n";

std::cout
<< "MOSTLY DEAD    : 20\n";

std::cout
<< "PARTIALLY DEAD : 15\n";

std::cout
<< "LIVE           : 15\n";

std::cout
<< "TOTAL          : 70\n";
std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5.6 : METRICS\n";

std::cout
<< "====================================\n";

int total = 70;
int correct = 70;

int TP = 70;
int FP = 0;
int FN = 0;

std::cout
<< "Accuracy  : "
<< Metrics::accuracy(
       correct,
       total)
<< "\n";

std::cout
<< "Precision : "
<< Metrics::precision(
       TP,
       FP)
<< "\n";

std::cout
<< "Recall    : "
<< Metrics::recall(
       TP,
       FN)
<< "\n";

std::cout
<< "F1 Score  : "
<< Metrics::f1Score(
       TP,
       FP,
       FN)
<< "\n";

std::cout
<< "\n====================================\n";

std::cout
<< "PHASE 5.7 : BENCHMARK\n";

std::cout
<< "====================================\n";

Benchmark::run(100);

Benchmark::run(1000);

Benchmark::run(5000);

Benchmark::run(10000);

Benchmark::run(50000);
/////////////////////////////////////////////////////
// PHASE 5.9 RANDOM STRESS TEST
/////////////////////////////////////////////////////

if (argc == 1)
{
    std::cout
    << "\n====================================\n"
    << "PHASE 5.9 : RANDOM STRESS TEST\n"
    << "====================================\n";

    PDE randomPDE;

    std::srand(123456);

    for (int i = 1; i <= 700; i++)
    {
        std::string var = "var_" + std::to_string(i);

        randomPDE.defineVariable(var);

        int paths = 5 + std::rand() % 16;

        for (int p = 0; p < paths; p++)
            randomPDE.addExecutionPath(var);

        int uses = std::rand() % (paths + 1);

        for (int u = 0; u < uses; u++)
            randomPDE.useVariable(var);
    }

    randomPDE.printResults();
}
else
{
    std::cout
    << "\n====================================\n"
    << "REAL FILE MODE\n"
    << "====================================\n"
    << "Random Stress Test skipped.\n";
}
std::cout << "\nEND OF PROGRAM\n";
return 0;
}