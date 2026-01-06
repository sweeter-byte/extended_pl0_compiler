#include "Instruction.h"
#include "Common.h"
#include <iostream>
#include <iomanip>

namespace pl0 {

CodeGenerator::CodeGenerator() {}

int CodeGenerator::emit(OpCode op, int L, int A, int line) {
    int addr = static_cast<int>(code_.size());
    code_.emplace_back(op, L, A, line);
    return addr;
}

void CodeGenerator::backpatch(int instrAddr, int targetAddr) {
    if (instrAddr >= 0 && instrAddr < static_cast<int>(code_.size())) {
        code_[instrAddr].A = targetAddr;
    }
}

void CodeGenerator::dump() const {
    std::cout << "\n" << Color::Cyan << "[P-Code]" << Color::Reset 
              << " Generated Instructions:\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (size_t i = 0; i < code_.size(); i++) {
        const auto& instr = code_[i];
        std::cout << std::setw(4) << i << ": "
                  << "L" << std::setw(3) << instr.line << " "
                  << std::setw(4) << opCodeToString(instr.op) << " "
                  << std::setw(3) << instr.L << ", "
                  << std::setw(5) << instr.A;
        
        // Add comment
        std::cout << "    " << Color::Green << "; ";
        switch (instr.op) {
            case OpCode::INT:
                std::cout << "allocate " << instr.A << " units";
                break;
            case OpCode::LIT:
                std::cout << "push constant " << instr.A;
                break;
            case OpCode::LOD:
                if (instr.A == 0) {
                    std::cout << "indirect load";
                } else {
                    std::cout << "load [" << instr.L << ", " << instr.A << "]";
                }
                break;
            case OpCode::STO:
                if (instr.A == 0) {
                    std::cout << "indirect store";
                } else {
                    std::cout << "store to [" << instr.L << ", " << instr.A << "]";
                }
                break;
            case OpCode::CAL:
                std::cout << "call @" << instr.A;
                break;
            case OpCode::JMP:
                std::cout << "jump to " << instr.A;
                break;
            case OpCode::JPC:
                std::cout << "jump if zero to " << instr.A;
                break;
            case OpCode::OPR:
                std::cout << oprCodeToString(static_cast<OprCode>(instr.A));
                break;
            case OpCode::RED:
                if (instr.A == 0) {
                    std::cout << "read indirect";
                } else {
                    std::cout << "read to [" << instr.L << ", " << instr.A << "]";
                }
                break;
            case OpCode::WRT:
                std::cout << "write";
                break;
            case OpCode::NEW:
                std::cout << "heap alloc";
                break;
            case OpCode::DEL:
                std::cout << "heap free";
                break;
            case OpCode::LAD:
                std::cout << "load address";
                break;
        }
        std::cout << Color::Reset << "\n";
    }
    
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Total instructions: " << code_.size() << "\n";
}

const char* opCodeToString(OpCode op) {
    switch (op) {
        case OpCode::LIT: return "LIT";
        case OpCode::LOD: return "LOD";
        case OpCode::STO: return "STO";
        case OpCode::CAL: return "CAL";
        case OpCode::INT: return "INT";
        case OpCode::JMP: return "JMP";
        case OpCode::JPC: return "JPC";
        case OpCode::OPR: return "OPR";
        case OpCode::RED: return "RED";
        case OpCode::WRT: return "WRT";
        case OpCode::NEW: return "NEW";
        case OpCode::DEL: return "DEL";
        case OpCode::LAD: return "LAD";
        default: return "???";
    }
}

const char* oprCodeToString(OprCode opr) {
    switch (opr) {
        case OprCode::RET: return "return";
        case OprCode::NEG: return "negate";
        case OprCode::ADD: return "add";
        case OprCode::SUB: return "subtract";
        case OprCode::MUL: return "multiply";
        case OprCode::DIV: return "divide";
        case OprCode::ODD: return "odd";
        case OprCode::MOD: return "modulo";
        case OprCode::EQL: return "equal";
        case OprCode::NEQ: return "not equal";
        case OprCode::LSS: return "less than";
        case OprCode::GEQ: return "greater or equal";
        case OprCode::GTR: return "greater than";
        case OprCode::LEQ: return "less or equal";
        default: return "???";
    }
}

} // namespace pl0
