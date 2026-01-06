#ifndef PL0_SYMBOL_TABLE_H
#define PL0_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <list>

namespace pl0 {

// Symbol kind enumeration
enum class SymbolKind {
    CONSTANT,       // Constant
    VARIABLE,       // Variable
    PROCEDURE,      // Procedure
    ARRAY,          // Array
    POINTER         // Pointer
};

// Symbol table entry
struct Symbol {
    std::string name;       // Identifier name
    SymbolKind kind;        // Symbol kind
    int level;              // Definition level (0 = main program)
    int address;            // Address/offset
                            //   CONSTANT: not used
                            //   VARIABLE: stack frame offset (starts from 3)
                            //   ARRAY: array base stack frame offset
                            //   PROCEDURE: code entry address

    int value;              // CONSTANT: constant value
    int size;               // ARRAY: array size
    int paramCount;         // PROCEDURE: parameter count
    
    int tableIndex;         // Index in symbol stack (for fast deletion)
    int historyIndex;       // Index in allSymbols_ (for dump sync)

    Symbol() : kind(SymbolKind::VARIABLE), level(0), address(0), 
               value(0), size(0), paramCount(0), tableIndex(-1), historyIndex(-1) {}
    
    Symbol(const std::string& n, SymbolKind k, int lv, int addr)
        : name(n), kind(k), level(lv), address(addr), 
          value(0), size(0), paramCount(0), tableIndex(-1), historyIndex(-1) {}
};

// Hash table entry: list of indices for symbols with same name
struct HashEntry {
    std::list<int> indices;  // Indices into symbol stack (front = innermost scope)
};

// Symbol table manager class (Stack + Hash Table implementation)
class SymbolTable {
public:
    SymbolTable();

    // Enter new scope (level + 1)
    void enterScope();
    
    // Leave scope (pop all symbols from current level, update hash table)
    void leaveScope();
    
    // Get current level
    int getCurrentLevel() const { return currentLevel_; }

    // Returns: index in symbol stack, -1 on failure (duplicate definition)
    int registerSymbol(const std::string& name, SymbolKind kind, int address);
    
    // Returns: index in symbol stack, -1 if not found
    int lookup(const std::string& name) const;
    
    // Lookup only in current scope (for detecting duplicate definitions)
    int lookupCurrentScope(const std::string& name) const;
    
    // Check if symbol exists
    bool exists(const std::string& name) const;

    Symbol& getSymbol(int index);
    const Symbol& getSymbol(int index) const;
    
    void updateSymbolAddress(int index, int address);
    void updateSymbolParamCount(int index, int paramCount);
    void updateSymbolSize(int index, int size);
    void updateSymbolValue(int index, int value);
    
    // Get symbol stack size
    int getTableSize() const { return static_cast<int>(symbolStack_.size()); }
    
    // Debug API: Access all recorded symbols
    const std::vector<Symbol>& getAllSymbols() const { return allSymbols_; }

    // Debug Output
    void dump() const;
    void dumpHashTable() const;

private:
    void removeFromHashTable(const std::string& name, int index);
    void addToHashTable(const std::string& name, int index);

    std::vector<Symbol> symbolStack_;
    
    // Complete symbol history: stores ALL symbols ever registered (for dump)
    std::vector<Symbol> allSymbols_;
    
    // Hash table: name -> list of symbol indices
    // List head is the innermost (newest) symbol
    std::unordered_map<std::string, HashEntry> hashTable_;
    
    // Scope stack: records start index for each level
    std::vector<int> scopeStack_;
    
    // Current level
    int currentLevel_;
};

const char* symbolKindToString(SymbolKind kind);

} // namespace pl0

#endif // PL0_SYMBOL_TABLE_H
