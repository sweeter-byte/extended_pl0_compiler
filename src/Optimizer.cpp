#include "Optimizer.h"
#include <stack>
#include <iostream>

namespace pl0 {

Optimizer::Optimizer() {}

std::vector<Instruction> Optimizer::optimize(const std::vector<Instruction>& input) {
    blocks_.clear();
    addressMap_.clear();
    std::set<int> targets;
    
    // 1. Analysis (Initial partitioning)
    analyzeJumpTargets(input, targets);
    buildBasicBlocks(input, targets);

    // 2. Optimization (Local)
    for (auto& block : blocks_) {
        constantFolding(block);
        strengthReduction(block);
    }
    
    // 3. Analysis (Post-optimization CFG)
    buildCFG();
    markReachable(0);
    
    // 4. Reconstruction
    return flattenAndRemap();
}

void Optimizer::analyzeJumpTargets(const std::vector<Instruction>& code, std::set<int>& targets) {
    for (const auto& instr : code) {
        if (instr.op == OpCode::JMP || instr.op == OpCode::JPC) {
            targets.insert(instr.A);
        }
    }
}

void Optimizer::buildBasicBlocks(const std::vector<Instruction>& code, const std::set<int>& targets) {
    if (code.empty()) return;


    BasicBlock currentBlock;
    currentBlock.id = 0;
    currentBlock.originalStartAddr = 0;
    
    for (size_t i = 0; i < code.size(); ++i) {
        // Check if we need to start a new block
        bool split = false;
        
        // 1. Current address is a jump target
        if (i > 0 && targets.count(static_cast<int>(i))) {
            split = true;
        }
        
        // 2. Previous instruction was a terminator
        if (i > 0) {
            OpCode prevOp = code[i-1].op;
            if (prevOp == OpCode::JMP || prevOp == OpCode::JPC) {
                split = true;
            } else if (prevOp == OpCode::OPR && static_cast<OprCode>(code[i-1].A) == OprCode::RET) {
                split = true;
            }
        }
        
        if (split) {
            blocks_.push_back(currentBlock);
            currentBlock = BasicBlock();
            currentBlock.id = blocks_.size();
            currentBlock.originalStartAddr = static_cast<int>(i);
        }
        
        currentBlock.instructions.push_back(code[i]);
    }
    
    blocks_.push_back(currentBlock);
}

void Optimizer::buildCFG() {
    std::map<int, int> addrToBlock;
    for (const auto& b : blocks_) {
        addrToBlock[b.originalStartAddr] = b.id;
    }
    
    for (auto& block : blocks_) {
        if (block.instructions.empty()) continue;
        
        Instruction last = block.instructions.back();
        bool fallsThrough = true;
        
        if (last.op == OpCode::JMP) {
            fallsThrough = false;
            if (addrToBlock.count(last.A)) {
                block.successors.push_back(addrToBlock[last.A]);
            }
        } else if (last.op == OpCode::JPC) {
            fallsThrough = true;
            if (addrToBlock.count(last.A)) {
                block.successors.push_back(addrToBlock[last.A]);
            }
        } else if (last.op == OpCode::OPR && static_cast<OprCode>(last.A) == OprCode::RET) {
            fallsThrough = false;
        }
        
        if (fallsThrough) {
             // Fallthrough to next block in sequence
             if (block.id + 1 < static_cast<int>(blocks_.size())) {
                 block.successors.push_back(block.id + 1);
             }
        }
    }
}

void Optimizer::markReachable(int startBlockId) {
    if (startBlockId >= static_cast<int>(blocks_.size())) return;
    
    std::vector<int> q;
    q.push_back(startBlockId);
    blocks_[startBlockId].reachable = true;
    
    size_t head = 0;
    while(head < q.size()){
        int curr = q[head++];
        for (int succ : blocks_[curr].successors) {
            if (succ < static_cast<int>(blocks_.size()) && !blocks_[succ].reachable) {
                blocks_[succ].reachable = true;
                q.push_back(succ);
            }
        }
    }
}

void Optimizer::constantFolding(BasicBlock& block) {
    std::vector<Instruction>& insts = block.instructions;
    
    bool changed = true;
    while (changed) {
        changed = false;
        if (insts.size() < 3) break;
        
        std::vector<Instruction> optim;
        size_t i = 0;
        while (i < insts.size()) {
            if (i + 2 < insts.size()) {
                if (insts[i].op == OpCode::LIT && 
                    insts[i+1].op == OpCode::LIT && 
                    insts[i+2].op == OpCode::OPR) {
                        
                    int v1 = insts[i].A;
                    int v2 = insts[i+1].A;
                    int opr = insts[i+2].A;
                    int res = 0;
                    bool valid = true;
                    
                    switch(static_cast<OprCode>(opr)) {
                        case OprCode::ADD: res = v1 + v2; break;
                        case OprCode::SUB: res = v1 - v2; break;
                        case OprCode::MUL: res = v1 * v2; break;
                        case OprCode::DIV: 
                            if(v2!=0) res = v1 / v2; else valid=false; 
                            break;
                        case OprCode::EQL: res = (v1 == v2); break;
                        case OprCode::NEQ: res = (v1 != v2); break;
                        case OprCode::LSS: res = (v1 < v2); break;
                        case OprCode::GEQ: res = (v1 >= v2); break;
                        case OprCode::GTR: res = (v1 > v2); break;
                        case OprCode::LEQ: res = (v1 <= v2); break;
                        default: valid = false;
                    }
                    
                    if (valid) {
                        optim.push_back(Instruction(OpCode::LIT, 0, res));
                        i += 3;
                        changed = true;
                        continue;
                    }
                }
            }
            optim.push_back(insts[i]);
            i++;
        }
        insts = optim;
    }
}

void Optimizer::strengthReduction(BasicBlock& block) {
    std::vector<Instruction> optim;
    std::vector<Instruction>& insts = block.instructions;
    
    size_t i = 0;
    while (i < insts.size()) {
        bool reduced = false;
        if (i + 1 < insts.size()) {
            if (insts[i].op == OpCode::LIT && insts[i+1].op == OpCode::OPR) {
                int litVal = insts[i].A;
                int opr = insts[i+1].A;
                
                // x + 0 -> x
                if (litVal == 0 && static_cast<OprCode>(opr) == OprCode::ADD) reduced = true;
                // x - 0 -> x
                if (litVal == 0 && static_cast<OprCode>(opr) == OprCode::SUB) reduced = true;
                // x * 1 -> x
                if (litVal == 1 && static_cast<OprCode>(opr) == OprCode::MUL) reduced = true;
                // x / 1 -> x
                if (litVal == 1 && static_cast<OprCode>(opr) == OprCode::DIV) reduced = true; 
                
                if (reduced) {
                    i += 2;
                    continue;
                }
            } else if (insts[i].op == OpCode::LIT && insts[i+1].op == OpCode::JPC) {
                // JPC jumps if Top == 0 (False)
                int litVal = insts[i].A;
                
                if (litVal == 0) { // False -> Always Jump
                    // LIT 0, JPC target -> JMP target
                    optim.push_back(Instruction(OpCode::JMP, insts[i+1].L, insts[i+1].A));
                    i += 2;
                    continue;
                } else { // True -> Never Jump
                    // LIT 1, JPC target -> Remove (Fallthrough)
                    i += 2;
                    continue;
                }
            }
        }
        optim.push_back(insts[i]);
        i++;
    }
    insts = optim;
}

std::vector<Instruction> Optimizer::flattenAndRemap() {
    std::vector<Instruction> result;
    addressMap_.clear();
    
    // Pass 1: Assign new addresses
    int currentAddr = 0;
    for (const auto& block : blocks_) {
        if (!block.reachable) continue;
        
        addressMap_[block.originalStartAddr] = currentAddr;
        currentAddr += block.instructions.size();
    }
    
    // Pass 2: Emit and Remap
    for (const auto& block : blocks_) {
        if (!block.reachable) continue;
        
        for (auto instr : block.instructions) {
            if (instr.op == OpCode::JMP || instr.op == OpCode::JPC) {
                if (addressMap_.count(instr.A)) {
                    instr.A = addressMap_[instr.A];
                }
            }
            result.push_back(instr);
        }
    }
    
    return result;
}

} // namespace pl0
