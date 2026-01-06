#include "SymbolTable.h"
#include "Common.h"
#include <iostream>
#include <iomanip>
#include <cassert>

namespace pl0 {

SymbolTable::SymbolTable() : currentLevel_(0) {
    // Initialize: Level 0 starts at index 0
    scopeStack_.push_back(0);
}

void SymbolTable::enterScope() {
    currentLevel_++;
    // Record start index for new scope
    scopeStack_.push_back(static_cast<int>(symbolStack_.size()));
}

void SymbolTable::leaveScope() {
    if (currentLevel_ == 0) {
        // Cannot leave main program scope
        return;
    }
    
    // Get start index for current scope
    int scopeStart = scopeStack_.back();
    scopeStack_.pop_back();
    
    // Remove all symbols from current level and update hash table
    while (static_cast<int>(symbolStack_.size()) > scopeStart) {
        Symbol& sym = symbolStack_.back();
        
        // Remove from hash table
        removeFromHashTable(sym.name, sym.tableIndex);
        
        // Pop from symbol stack
        symbolStack_.pop_back();
    }
    
    currentLevel_--;
}

// Symbol Operations 

int SymbolTable::registerSymbol(const std::string& name, SymbolKind kind, int address) {
    // Check for duplicate definition in current scope
    if (lookupCurrentScope(name) >= 0) {
        return -1;  // Duplicate definition
    }
    
    // Create new symbol
    Symbol sym(name, kind, currentLevel_, address);
    sym.tableIndex = static_cast<int>(symbolStack_.size());
    sym.historyIndex = static_cast<int>(allSymbols_.size());
    
    // Add to symbol stack
    symbolStack_.push_back(sym);
    
    // Add to complete history (for dump output)
    allSymbols_.push_back(sym);
    
    // Add to hash table
    addToHashTable(name, sym.tableIndex);
    
    return sym.tableIndex;
}

int SymbolTable::lookup(const std::string& name) const {
    // O(1) average complexity lookup
    auto it = hashTable_.find(name);
    if (it == hashTable_.end() || it->second.indices.empty()) {
        return -1;  // Not found
    }
    
    // Return head of list (innermost scope symbol)
    return it->second.indices.front();
}

int SymbolTable::lookupCurrentScope(const std::string& name) const {
    auto it = hashTable_.find(name);
    if (it == hashTable_.end() || it->second.indices.empty()) {
        return -1;
    }
    
    // Check if innermost symbol is in current scope
    int index = it->second.indices.front();
    if (symbolStack_[index].level == currentLevel_) {
        return index;
    }
    
    return -1;  // Not in current scope
}

bool SymbolTable::exists(const std::string& name) const {
    return lookup(name) >= 0;
}

// Hash Table Operations 

void SymbolTable::addToHashTable(const std::string& name, int index) {
    // Add new index to front of list (innermost priority)
    hashTable_[name].indices.push_front(index);
}

void SymbolTable::removeFromHashTable(const std::string& name, int index) {
    auto it = hashTable_.find(name);
    if (it == hashTable_.end()) {
        return;
    }
    
    // Remove specified index from list
    it->second.indices.remove(index);
    
    // Optionally remove empty entry
    if (it->second.indices.empty()) {
        hashTable_.erase(it);
    }
}

// Symbol Access 

Symbol& SymbolTable::getSymbol(int index) {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    return symbolStack_[index];
}

const Symbol& SymbolTable::getSymbol(int index) const {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    return symbolStack_[index];
}

void SymbolTable::updateSymbolAddress(int index, int address) {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    symbolStack_[index].address = address;
    
    // Also update in history for dump output
    int histIdx = symbolStack_[index].historyIndex;
    if (histIdx >= 0 && histIdx < static_cast<int>(allSymbols_.size())) {
        allSymbols_[histIdx].address = address;
    }
}

void SymbolTable::updateSymbolParamCount(int index, int paramCount) {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    symbolStack_[index].paramCount = paramCount;
    
    // Also update in history for dump output
    int histIdx = symbolStack_[index].historyIndex;
    if (histIdx >= 0 && histIdx < static_cast<int>(allSymbols_.size())) {
        allSymbols_[histIdx].paramCount = paramCount;
    }
}

void SymbolTable::updateSymbolSize(int index, int size) {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    symbolStack_[index].size = size;
    
    // Also update in history for dump output
    int histIdx = symbolStack_[index].historyIndex;
    if (histIdx >= 0 && histIdx < static_cast<int>(allSymbols_.size())) {
        allSymbols_[histIdx].size = size;
    }
}

void SymbolTable::updateSymbolValue(int index, int value) {
    assert(index >= 0 && index < static_cast<int>(symbolStack_.size()));
    symbolStack_[index].value = value;
    
    // Also update in history for dump output
    int histIdx = symbolStack_[index].historyIndex;
    if (histIdx >= 0 && histIdx < static_cast<int>(allSymbols_.size())) {
        allSymbols_[histIdx].value = value;
    }
}

// Debug Output 

const char* symbolKindToString(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::CONSTANT:  return "CONST";
        case SymbolKind::VARIABLE:  return "VAR";
        case SymbolKind::ARRAY:     return "ARRAY";
        case SymbolKind::PROCEDURE: return "PROC";
        default:                    return "???";
    }
}

void SymbolTable::dump() const {
    std::cout << "\n";
    std::cout << Color::Cyan << "[Symbol Table]" << Color::Reset 
              << " Stack + Hash Implementation\n";
    std::cout << std::string(76, '-') << "\n";
    std::cout << std::left 
              << "| " << std::setw(5) << "Index"
              << "| " << std::setw(15) << "Name"
              << "| " << std::setw(8) << "Kind"
              << "| " << std::setw(6) << "Level"
              << "| " << std::setw(12) << "Addr/Val"
              << "| " << std::setw(12) << "Size/Params"
              << "|\n";
    std::cout << std::string(76, '-') << "\n";
    
    // Use allSymbols_ to show complete symbol history
    for (size_t i = 0; i < allSymbols_.size(); i++) {
        const Symbol& sym = allSymbols_[i];
        std::cout << "| " << std::left << std::setw(5) << i;
        std::cout << "| " << std::setw(15) << sym.name;
        std::cout << "| " << std::setw(8) << symbolKindToString(sym.kind);
        std::cout << "| " << std::setw(6) << sym.level;
        
        switch (sym.kind) {
            case SymbolKind::CONSTANT:
                std::cout << "| " << std::setw(12) << sym.value
                          << "| " << std::setw(12) << "-";
                break;
            case SymbolKind::VARIABLE:
                std::cout << "| " << std::setw(12) << sym.address
                          << "| " << std::setw(12) << "-";
                break;
            case SymbolKind::ARRAY:
                std::cout << "| " << std::setw(12) << sym.address
                          << "| " << std::setw(12) << sym.size;
                break;
            case SymbolKind::PROCEDURE:
                std::cout << "| " << std::setw(12) << sym.address
                          << "| " << std::setw(12) << sym.paramCount;
                break;
        }
        std::cout << "|\n";
    }
    
    std::cout << std::string(76, '-') << "\n";
    std::cout << "Total symbols: " << allSymbols_.size() << "\n";
}

void SymbolTable::dumpHashTable() const {
    std::cout << "\n" << Color::Cyan << "[Hash Table]" << Color::Reset << " State:\n";
    std::cout << std::string(50, '-') << "\n";
    
    for (const auto& [name, entry] : hashTable_) {
        std::cout << "  \"" << name << "\" -> [";
        bool first = true;
        for (int idx : entry.indices) {
            if (!first) std::cout << " -> ";
            std::cout << idx << "(L" << symbolStack_[idx].level << ")";
            first = false;
        }
        std::cout << "]\n";
    }
    std::cout << std::string(50, '-') << "\n";
}

} // namespace pl0
