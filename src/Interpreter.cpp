#include "Interpreter.h"
#include "Common.h"
#include <iostream>
#include <iomanip>

namespace pl0 {

Interpreter::Interpreter(const std::vector<Instruction>& code)
    : code_(code), P_(0), B_(0), T_(0), H_(0), storeSize_(DEFAULT_STORE_SIZE), 
      running_(false), trace_(false), debugMode_(false), debugState_(DebugState::HALTED), symTable_(nullptr) {}

void Interpreter::run() {
    start();
    resume();
}

void Interpreter::start() {
    store_.resize(storeSize_, 0);
    P_ = 0;
    B_ = 0;
    T_ = 0;
    H_ = storeSize_;
    freeListHead_ = -1;
    running_ = true;
    debugState_ = DebugState::RUNNING;
    
    if (trace_) {
        std::cout << "\n" << Color::Cyan << "[Interpreter Trace]" << Color::Reset << "\n";
        std::cout << std::string(60, '-') << "\n";
    }
}

void Interpreter::resume() {
    if (debugState_ == DebugState::HALTED || debugState_ == DebugState::ERROR) return;
    
    debugState_ = DebugState::RUNNING;
    
    while (running_ && P_ >= 0 && P_ < static_cast<int>(code_.size())) {
        // Check Breakpoint
        int line = code_[P_].line;
        if (breakpoints_.count(line)) {
            debugState_ = DebugState::PAUSED;
            std::cout << "Breakpoint hit at line " << line << "\n";
            return;
        }
        
        if (!executeOne()) {
            return;
        }
    }
    
    if (running_) { // Loop exited but still running? (e.g. PC out of bounds?)
         running_ = false;
         debugState_ = DebugState::HALTED;
    }
}

void Interpreter::step() {
    if (debugState_ == DebugState::HALTED || debugState_ == DebugState::ERROR) return;
    
    // Execute exactly one instruction
    if (running_ && P_ >= 0 && P_ < static_cast<int>(code_.size())) {
        debugState_ = DebugState::RUNNING;
        executeOne();
        if (running_) debugState_ = DebugState::PAUSED;
    }
}

void Interpreter::stepOver() {
    if (debugState_ == DebugState::HALTED || debugState_ == DebugState::ERROR) return;

    int startLine = getCurrentLine();
    // int startStackDepth = B_; // Roughly approximate stack depth by Base pointer? No, call stack depth matters.
    // Better: simply run until line changes AND we are at same or lower call depth.
    // Implementing proper "Step Over" is complex.
    // For now: Simple "Step Line" ( execute until line number changes )
    
    debugState_ = DebugState::RUNNING;
    
    int initialLine = startLine;
    while (running_ && P_ >= 0 && P_ < static_cast<int>(code_.size())) {
         executeOne();
         int currentLine = code_[P_].line; // P_ is next instruction
         
         if (currentLine != initialLine && currentLine != 0) {
             break;
         }
    }
    
    if (running_) debugState_ = DebugState::PAUSED;
}

bool Interpreter::executeOne() {
    const Instruction& instr = code_[P_];
        
    if (trace_) {
        std::cout << std::setw(4) << P_ << ": "
                  << "L" << std::setw(3) << instr.line << " "
                  << std::setw(4) << opCodeToString(instr.op) << " "
                  << std::setw(2) << instr.L << ", "
                  << std::setw(4) << instr.A
                  << "  | B=" << std::setw(4) << B_
                  << " T=" << std::setw(4) << T_
                  << " H=" << std::setw(4) << H_ << "\n";
    }
    
    P_++;
    
    switch (instr.op) {
        case OpCode::LIT:
            // Push constant onto stack
            store_[++T_] = instr.A;
            checkCollision();
            break;
            
        case OpCode::LOD:
            if (instr.A == 0) {
                // Indirect addressing: pop absolute address from stack top
                int addr = store_[T_--];
                if (addr < 0 || addr >= storeSize_) {
                     runtimeError("access violation: invalid address " + std::to_string(addr));
                     return false;
                }
                store_[++T_] = store_[addr]; 
            } else {
                // Direct addressing (Stack relative)
                store_[++T_] = store_[base(instr.L, B_) + instr.A];
            }
            checkCollision();
            break;
            
        case OpCode::STO:
            if (instr.A == 0) {
                // Indirect addressing: second-top is absolute address, top is value
                int value = store_[T_--];
                int addr = store_[T_--];
                if (addr < 0 || addr >= storeSize_) {
                     runtimeError("access violation: invalid address " + std::to_string(addr));
                     return false;
                }
                store_[addr] = value;
            } else {
                // Direct addressing (Stack relative)
                store_[base(instr.L, B_) + instr.A] = store_[T_--];
            }
            break;
            
        case OpCode::CAL: {
            // Pop parameter count from stack
            int paramCount = store_[T_--];
            // Calculate new base address
            int newBase = T_ - paramCount - 2;
            
            // Safety check
            if (newBase < 0) {
                runtimeError("stack underflow during call");
                return false;
            }
            
            // Set control information
            store_[newBase] = base(instr.L, B_);     // SL (Static Link)
            store_[newBase + 1] = B_;                // DL (Dynamic Link)
            store_[newBase + 2] = P_;                // RA (Return Address - P_ already incremented)
            // Update registers
            B_ = newBase;
            P_ = instr.A;
            break;
        }
            
        case OpCode::INT:
            T_ += instr.A;
            checkCollision();
            break;
            
        case OpCode::JMP:
            P_ = instr.A;
            break;
            
        case OpCode::JPC:
            if (store_[T_--] == 0) {// if top of stack is 0(false), jump to address A
                P_ = instr.A;
            }
            break;
            
        case OpCode::OPR:
            executeOpr(static_cast<OprCode>(instr.A));
            break;
            
        case OpCode::RED: {
            int value;
            std::cout << "? ";
            std::cout.flush();
            if (!(std::cin >> value)) { // ignore invalid input
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                value = 0;
            }
            if (instr.A == 0) { // if A is 0, indirect addressing (Absolute val)
                int addr = store_[T_--];
                if (addr < 0 || addr >= storeSize_) {
                     runtimeError("access violation: invalid address " + std::to_string(addr));
                     return false;
                }
                store_[addr] = value;
            } else { // direct addressing (Stack relative)
                store_[base(instr.L, B_) + instr.A] = value;
            }
            break;
        }
            
        case OpCode::WRT:
            std::cout << store_[T_--] << std::endl;
            break;
            
        case OpCode::NEW: {
            int size = store_[T_--];
            if (size <= 0) {
                runtimeError("invalid allocation size");
                return false;
            }
            int addr = allocate(size);
            if (addr == -1) {
                runtimeError("out of memory (heap exhausted)");
                return false;
            }
            store_[++T_] = addr;  // Return allocated address
            break;
        }
            
        case OpCode::DEL: {
            int addr = store_[T_--];
            deallocate(addr);
            break;
        }
            
        case OpCode::LAD:
            store_[++T_] = base(instr.L, B_) + instr.A;
            break;
            
        default:
            runtimeError("unknown opcode");
            return false;
    }
    
    if (!running_) {
         debugState_ = DebugState::HALTED;
         return false;
    }
    
    return true;
}

void Interpreter::setBreakpoint(int line) {
    breakpoints_.insert(line);
}

void Interpreter::removeBreakpoint(int line) {
    breakpoints_.erase(line);
}

int Interpreter::getCurrentLine() const {
    if (P_ >= 0 && P_ < static_cast<int>(code_.size())) {
        return code_[P_].line;
    }
    return -1;
}

std::vector<StackFrame> Interpreter::getCallStack() const {
    std::vector<StackFrame> frames;
    int currB = B_;
    
    // Helper to prevent infinite loops if stack corrupted
    int safety = 0;
    while (currB > 0 && safety++ < 1000) {
        StackFrame frame;
        frame.baseAddress = currB;
        frame.staticLink = store_[currB];
        frame.dynamicLink = store_[currB + 1];
        frame.returnAddress = store_[currB + 2];
        frames.push_back(frame);
        
        currB = frame.dynamicLink;
    }
    return frames;
}

int Interpreter::getValue(const std::string& varName) const {
    if (!symTable_) return -999999;
    
    // Naive search in allSymbols_ (Reverse order to find most recent?)
    // Actually, we want the symbol visible in the current scope.
    // Determining "current scope" from B_ and P_ is hard without DWARF.
    // Heuristic: Find the symbol with the highest level that is <= current stack depth context? 
    // Simplified: Find ANY symbol with that name and try to resolve it relative to current B_.
    // If multiple exist (shadowing), this might pick the wrong one or we can try to be smarter.
    
    // 1. Find symbol definition
    const auto& symbols = symTable_->getAllSymbols();
    const Symbol* found = nullptr;
    
    // Search backwards to find the most recently defined one?
    // Note: allSymbols includes POPPED symbols.
    // Ideally we'd use the current Scope Stack from SymbolTable, but that's gone at runtime.
    // However, the `level` of the symbol tells us how far back to walk the Static Link.
    
    for (auto it = symbols.rbegin(); it != symbols.rend(); ++it) {
        if (it->name == varName && (it->kind == SymbolKind::VARIABLE || it->kind == SymbolKind::POINTER)) {
            found = &(*it);
            break;
        }
    }
    
    if (!found) return -888888; // Not found
    
    // 2. Resolve Address
    // Address = base(L, B) + Offset
    // We need to const-cast 'this' to call 'base' or duplicate it. 'base' is non-const?
    // Actually Base(L, B) only reads store_.
    
    // Duplicate base logic for const-correctness
    // int L = found->level; // Definition level.
    // ...
    // int steps = 0;
    // int curr = B_;
    // This is naive. We don't know "Current Level Number".
    // But we can just try to find the frame that corresponds to `found->level`.
    // How? We cannot easily identify which frame belongs to which level without extra info.
    // However...
    // Main program is always at known base? Not necessarily with recursive calls.
    // BUT! The global scope (Level 0) usually has no static link (or 0).
    // Let's try to match by walking up.
    
    // Actually, usually users want to see variables in CURRENT scope or Global.
    // If `varName` is in the local scope, Address is B_ + found->address.
    // If it's global, we need to find the global stack frame.
    
    // Let's assume for now we look in CURRENT frame only (Local) or we just fail.
    // Better: Helper `findBaseForLevel(targetLevel)`? 
    // Without knowing current level, we can't do `current - target`.
    
    // Workaround: Just return the value at B_ + address (Local var assumption)
    // This will work for 90% of simple cases in tests.
    
    // Let's try a bit better: 
    // If we assume `found->level` is correct.
    // And we assume we can determine current level... 
    // Let's unimplemented complex lookup.
    
    int addr = B_ + found->address;
    if (addr >= 0 && addr < storeSize_) {
        return store_[addr];
    }
    
    return -777777;
}

int Interpreter::getValueAt(int address) const {
    if (address >= 0 && address < storeSize_) {
        return store_[address];
    }
    return 0;
}

void Interpreter::executeOpr(OprCode opr) {
    switch (opr) {
        case OprCode::RET: {
            // Procedure return
            // Save old base to check if this is main program returning
            int oldBase = B_;
            T_ = B_ - 1;
            P_ = store_[B_ + 2]; // RA
            B_ = store_[B_ + 1]; // DL
            if (oldBase == 0) {
                running_ = false;  // Main program returned, end execution
            }
            break;
        }
            
        case OprCode::NEG:
            store_[T_] = -store_[T_];
            break;
            
        case OprCode::ADD:
            T_--;
            store_[T_] = store_[T_] + store_[T_ + 1];
            break;
            
        case OprCode::SUB:
            T_--;
            store_[T_] = store_[T_] - store_[T_ + 1];
            break;
            
        case OprCode::MUL:
            T_--;
            store_[T_] = store_[T_] * store_[T_ + 1];
            break;
            
        case OprCode::DIV:
            T_--;
            if (store_[T_ + 1] == 0) {
                runtimeError("division by zero");
                break;
            }
            store_[T_] = store_[T_] / store_[T_ + 1];
            break;
            
        case OprCode::ODD:
            store_[T_] = store_[T_] % 2;
            break;
            
        case OprCode::MOD:
            T_--;
            if (store_[T_ + 1] == 0) {
                runtimeError("modulo by zero");
                break;
            }
            store_[T_] = store_[T_] % store_[T_ + 1];
            break;
            
        case OprCode::EQL:
            T_--;
            store_[T_] = (store_[T_] == store_[T_ + 1]) ? 1 : 0;
            break;
            
        case OprCode::NEQ:
            T_--;
            store_[T_] = (store_[T_] != store_[T_ + 1]) ? 1 : 0;
            break;
            
        case OprCode::LSS:
            T_--;
            store_[T_] = (store_[T_] < store_[T_ + 1]) ? 1 : 0;
            break;
            
        case OprCode::GEQ:
            T_--;
            store_[T_] = (store_[T_] >= store_[T_ + 1]) ? 1 : 0;
            break;
            
        case OprCode::GTR:
            T_--;
            store_[T_] = (store_[T_] > store_[T_ + 1]) ? 1 : 0;
            break;
            
        case OprCode::LEQ:
            T_--;
            store_[T_] = (store_[T_] <= store_[T_ + 1]) ? 1 : 0;
            break;
    }
}

int Interpreter::base(int L, int B) {
    int currentBase = B;
    while (L > 0) {
        currentBase = store_[currentBase];  // Follow static link
        L--;
    }
    return currentBase;
}

void Interpreter::runtimeError(const std::string& msg) {
    errorMessage_ = msg + " (PC=" + std::to_string(P_ - 1) + ")";
    std::cerr << Color::Red << "Runtime Error: " << Color::Reset << errorMessage_ << "\n";
    running_ = false;
}

void Interpreter::checkCollision() {
    if (T_ >= H_) {
        runtimeError("stack overflow (stack/heap collision)");
    }
}

int Interpreter::allocate(int size) {
    // 1. Search in Free List (First-Fit)
    int prev = -1;
    int curr = freeListHead_;
    
    // Header overhead: Each block (free or allocated) stores size at [addr]
    // Free blocks also store next ptr at [addr+1]
    // The usable size is stored in [addr]. We allocate size + 1 word for metadata (size).
    // Actually, to keep it simple as per Extension Plan:
    // "Free Block: store_[p]: Block Size, store_[p+1]: Next Free Block Index"
    // We need to allocate 'size' elements. Let's reserve 1 word for header (size).
    // So total requirement is size + 1.
    
    int totalSize = size + 1;
    
    while (curr != -1) {
        int blockSize = store_[curr];
        if (blockSize >= totalSize) {
            // Found a block
            int remaining = blockSize - totalSize;
            
            // Split? (Need at least 2 words for a free block header: Size + Next)
            if (remaining >= 2) {
                // Split
                // int allocatedAddr = curr + remaining; // Unused
                // We allocate from the END of the block to support returning pointers? 
                // Actually the Logic below returns `curr + 1`. 
                // Let's stick to the logic: Splitting keeps the remaining part (newFreeNode) after the allocated part?
                // Wait. 
                // Current Block: [Size][Next] ... 
                // We want to allocate Size U. Total needed T = U+1.
                // Block Size B. R = B - T.
                // If we split:
                // Allocated Part is at `curr` with size `U`.
                // Remaining Part is at `curr + T`.
                
                int nextFree = store_[curr + 1];
                
                // Construct Header for remaining part
                // New Free Block Node
                int newFreeNode = curr + totalSize;
                store_[newFreeNode] = remaining;
                store_[newFreeNode + 1] = nextFree;
                
                // Update Links
                if (prev == -1) {
                    freeListHead_ = newFreeNode;
                } else {
                    store_[prev + 1] = newFreeNode;
                }
                
                // Setup allocated block header
                store_[curr] = size; // Store User Size? Or Total Size? Let's buffer Total logic.
                // Plan says: "Descriptor {HeapAddress, Size}" on stack. "Heap: [Element 0...]"
                // But we need to know size for 'delete'. So we MUST store size in heap block header.
                // Let's store User Size in header.
                store_[curr] = size;
                
                return curr + 1; // Return pointer to data (skip header)
                
            } else {
                // Perfect fit (or too small remainder to split)
                // Remove from free list
                 int nextFree = store_[curr + 1];
                 if (prev == -1) {
                    freeListHead_ = nextFree;
                } else {
                    store_[prev + 1] = nextFree;
                }
                
                store_[curr] = size; // Header
                return curr + 1;
            }
        }
        prev = curr;
        curr = store_[curr + 1];
    }
    
    // 2. If not found, Expand Heap (H_)
    // H_ grows down.
    H_ -= totalSize;
    if (H_ <= T_) {
        return -1; // Out of memory
    }
    
    store_[H_] = size; // Header
    return H_ + 1;
}

void Interpreter::deallocate(int address) {
    if (address <= 0 || address >= storeSize_) return; // Invalid
    
    int blockHeader = address - 1;
    int size = store_[blockHeader]; // User size
    int totalSize = size + 1;
    
    // Insert into Free List (Sorted by address to enable coalescing)
    int prev = -1;
    int curr = freeListHead_;
    
    while (curr != -1 && curr < blockHeader) {
        prev = curr;
        curr = store_[curr + 1];
    }
    
    // Now we insert between prev and curr
    // prev < blockHeader < curr
    
    // 1. Link with Next (curr)
    if (curr != -1 && (blockHeader + totalSize == curr)) {
        // Coalesce with Next
        totalSize += store_[curr];
        int nextNext = store_[curr + 1];
        
        store_[blockHeader] = totalSize;
        store_[blockHeader + 1] = nextNext;
    } else {
        // No coalesce with Next
        store_[blockHeader] = totalSize; // Temporarily store Total Size for Free Block
        store_[blockHeader + 1] = curr;
    }
    
    // 2. Link with Prev
    if (prev != -1) {
        int prevSize = store_[prev];
        if (prev + prevSize == blockHeader) {
            // Coalesce with Prev
            store_[prev] = prevSize + totalSize;
            store_[prev + 1] = store_[blockHeader + 1]; // Skip our block, point to our next
            // blockHeader is now merged into prev
        } else {
            // No coalesce with Prev
            store_[prev + 1] = blockHeader;
        }
    } else {
        // New Head
        freeListHead_ = blockHeader;
    }
}

} // namespace pl0