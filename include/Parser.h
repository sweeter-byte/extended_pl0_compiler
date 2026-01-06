#ifndef PL0_PARSER_H
#define PL0_PARSER_H

#include "Lexer.h"
#include "SymbolTable.h"
#include "Instruction.h"
#include "Diagnostics.h"

namespace pl0 {

// Parser class
// Implements recursive descent parsing
class Parser {
public:
    Parser(Lexer& lexer, SymbolTable& symTable, CodeGenerator& codeGen, DiagnosticsEngine& diag);

    // Parse entry point
    bool parse();

    void enableAstDump(bool enable) { dumpAst_ = enable; }

private:
    void advance();                             
    bool check(TokenType type) const;           // Check current token type
    bool match(TokenType type);                 // Match and consume
    void expect(TokenType type, const char* msg); // Expect type, error otherwise
    void synchronize();                         // Error recovery
    
    void parseProgram();
    void parseBlock(int procIndex);             // procIndex: current procedure's index in symbol table
    void parseConstDecl();
    void parseVarDecl(int& dataOffset, std::vector<int>& arrayIndices);         // dataOffset: offset of first variable in block
    void parseProcDecl();
    void parseBody();
    void parseStatement();
    void parseIfStatement();
    void parseWhileStatement();
    void parseForStatement();
    void parseCallStatement();
    void parseReadStatement();
    void parseWriteStatement();
    void parseNewStatement();
    void parseDeleteStatement();
    void parseAssignOrArrayAssign();
    void parseCondition();                      // <lexp>
    void parseExpression();                     // <exp>
    void parseTerm();                           // <term>
    void parseFactor();                         // <factor>
    
    // Helper
    int emit(OpCode op, int L, int A);          // Wrapper around CodeGenerator::emit with line #
    void parseArrayElementAddress(Symbol& sym); // Handles array subscript, bounds check, and address calc

    // AST Debug Output 
    void astEnter(const std::string& nodeName);
    void astLeave();

    // Data Members
    Lexer& lexer_;
    SymbolTable& symTable_;
    CodeGenerator& codeGen_;
    DiagnosticsEngine& diag_;
    
    Token currentToken_;
    Token previousToken_;
    
    bool dumpAst_;
    int astIndent_;
    int currentTempOffset_; // Reserved for temporary calculations (e.g. bounds check)
};

} // namespace pl0

#endif // PL0_PARSER_H
