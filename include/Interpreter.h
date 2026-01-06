#include <vector>
#include <set>
#include <map>
#include "Instruction.h"
#include "SymbolTable.h"

namespace pl0 {

    enum class DebugState {
        RUNNING,
        PAUSED,
        HALTED,
        ERROR
    };

    struct StackFrame {
        int returnAddress;
        int dynamicLink;
        int staticLink;
        int baseAddress;
    };

    class Interpreter {
public:
    explicit Interpreter(const std::vector<Instruction>& code);

    // Execute program (Legacy/Batch mode)
    void run();

    // Debug API
    void setSymbolTable(const SymbolTable* symTable) { symTable_ = symTable; }
    void setDebugMode(bool debug) { debugMode_ = debug; }
    
    void setBreakpoint(int line);
    void removeBreakpoint(int line);
    
    void start(); // Reset and start
    void resume(); // Run until breakpoint
    void step();   // Execute one instruction
    void stepOver(); // Step one source line
    
    DebugState getDebugState() const { return debugState_; }
    int getCurrentLine() const;
    int getCurrentPC() const { return P_; }
    
    std::vector<StackFrame> getCallStack() const;
    int getValue(const std::string& varName) const;
    int getValueAt(int address) const;

    // Set store size
    void setStoreSize(int size) { storeSize_ = size; }

    // Enable debug trace
    void enableTrace(bool enable) { trace_ = enable; }

    // Check for runtime error
    bool hasError() const { return debugState_ == DebugState::ERROR || (!running_ && !errorMessage_.empty()); }

    // Get error message
    std::string getError() const { return errorMessage_; }

private:
    // Find base address for level difference L
    int base(int L, int B);

    // Execute OPR instruction
    void executeOpr(OprCode opr);

    // Runtime error handling
    void runtimeError(const std::string& msg);

    // Check stack/heap collision
    void checkCollision();

    // Heap Management (Free List)
    int allocate(int size);
    void deallocate(int address);
    
    bool executeOne(); // Returns true if should continue, false if halted/break

    const std::vector<Instruction>& code_;
    std::vector<int> store_;    // Unified data store (stack + heap)
    
    int P_;     // Program counter
    int B_;     // Base register
    int T_;     // Stack top pointer
    int H_;     // Heap pointer
    int freeListHead_; // Head of free list (sorted by address)
    
    int storeSize_;
    bool running_;
    bool trace_;
    std::string errorMessage_;
    
    // Debugger State
    bool debugMode_;
    DebugState debugState_;
    std::set<int> breakpoints_;
    const SymbolTable* symTable_;
};

} // namespace pl0


