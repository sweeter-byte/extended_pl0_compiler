#ifndef PL0_INSTRUCTION_H
#define PL0_INSTRUCTION_H

#include <vector>
#include <string>

namespace pl0 {

enum class OpCode {
    LIT,   
    LOD,   
    STO,   
    CAL,   
    INT,   
    JMP,   
    JPC,    
    OPR,    
    RED,    
    WRT,    
    NEW,    
    DEL,     
    LAD     
};

// OPR operation codes
enum class OprCode {
    RET = 0,    // Procedure return
    NEG = 1,    // Negate
    ADD = 2,    // Addition
    SUB = 3,    // Subtraction
    MUL = 4,    // Multiplication
    DIV = 5,    // Division
    ODD = 6,    // Odd test
    MOD = 7,    // Modulo
    EQL = 8,    // Equal
    NEQ = 9,    // Not equal
    LSS = 10,   // Less than
    GEQ = 11,   // Greater than or equal
    GTR = 12,   // Greater than
    LEQ = 13    // Less than or equal
};

// Instruction structure
struct Instruction {
    OpCode op;      // Operation code
    int L;          // Level difference
    int A;          // Operand/address
    int line;       // Source line number

    Instruction() : op(OpCode::LIT), L(0), A(0), line(0) {}
    Instruction(OpCode o, int l, int a, int ln = 0) : op(o), L(l), A(a), line(ln) {}
};

// Code generator class
class CodeGenerator {
public:
    CodeGenerator();

    // Emit instruction, returns instruction address
    int emit(OpCode op, int L, int A, int line = 0);

    // Backpatch jump address
    void backpatch(int instrAddr, int targetAddr);

    // Get next instruction address
    int getNextAddr() const { return static_cast<int>(code_.size()); }

    // Access instruction sequence
    const std::vector<Instruction>& getCode() const { return code_; }
    
    // Update instruction sequence (for optimization)
    void setCode(const std::vector<Instruction>& code) { code_ = code; }

    // Debug output
    void dump() const;

private:
    std::vector<Instruction> code_;
};

const char* opCodeToString(OpCode op);
const char* oprCodeToString(OprCode opr);

} // namespace pl0

#endif // PL0_INSTRUCTION_H
