#ifndef PL0_OPTIMIZER_H
#define PL0_OPTIMIZER_H

#include "Instruction.h"
#include <vector>
#include <map>
#include <set>

namespace pl0 {

struct BasicBlock {
    int id;
    int originalStartAddr; // For mapping
    std::vector<Instruction> instructions;
    std::vector<int> successors;
    bool reachable = false;
};

class Optimizer {
public:
    Optimizer();

    // Optimize the instruction sequence
    std::vector<Instruction> optimize(const std::vector<Instruction>& input);

private:
    // Analysis
    void analyzeJumpTargets(const std::vector<Instruction>& code, std::set<int>& targets);
    void buildBasicBlocks(const std::vector<Instruction>& code, const std::set<int>& targets);
    void buildCFG();
    void markReachable(int startBlockId);
    
    // Transformations
    void constantFolding(BasicBlock& block);
    void strengthReduction(BasicBlock& block);
    
    // Reconstruction
    std::vector<Instruction> flattenAndRemap();

    std::vector<BasicBlock> blocks_;
    // Map Old Address -> New Address (Only need to track Block Start addresses)
    std::map<int, int> addressMap_; 
};

} // namespace pl0

#endif // PL0_OPTIMIZER_H
