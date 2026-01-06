#include "Parser.h"
#include "Common.h"
#include <iostream>

namespace pl0 {

Parser::Parser(Lexer& lexer, SymbolTable& symTable, CodeGenerator& codeGen, DiagnosticsEngine& diag)
    : lexer_(lexer), symTable_(symTable), codeGen_(codeGen), diag_(diag), dumpAst_(false), astIndent_(0) {
    // Read first token
    advance();
}

void Parser::advance() {
    previousToken_ = currentToken_;
    currentToken_ = lexer_.nextToken();

    // Skip unknown tokens with error already reported by lexer
    while (currentToken_.type == TokenType::UNKNOWN) {
        currentToken_ = lexer_.nextToken();
    }
}

bool Parser::check(TokenType type) const {
    return currentToken_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::expect(TokenType type, const char* msg) {
    if (check(type)) {
        advance();
        return;
    }
    diag_.error(msg, currentToken_);
}


int Parser::emit(OpCode op, int L, int A) {
    return codeGen_.emit(op, L, A, previousToken_.line);
}

void Parser::synchronize() {
    // Skip tokens until synchronization point
    while (!check(TokenType::END_OF_FILE)) {
        // Statement terminator
        if (previousToken_.type == TokenType::DL_SEMICOLON) {
            return;
        }
        
        // Keywords that may start new statement
        switch (currentToken_.type) {
            case TokenType::KW_BEGIN:
            case TokenType::KW_END:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_CALL:
            case TokenType::KW_READ:
            case TokenType::KW_WRITE:
            case TokenType::KW_CONST:
            case TokenType::KW_VAR:
            case TokenType::KW_PROCEDURE:
                return;
            default:
                advance();
        }
    }
}

// AST Debug Output

void Parser::astEnter(const std::string& nodeName) {
    if (dumpAst_) {
        std::cout << std::string(astIndent_ * 2, ' ') 
                  << Color::Green << "+ " << nodeName << Color::Reset << "\n";
        astIndent_++;
    }
}

void Parser::astLeave() {
    if (dumpAst_) {
        astIndent_--;
    }
}

// Parse Entry 

bool Parser::parse() {
    parseProgram();
    
    // Strict check: Error if followed by a period
    if (check(TokenType::DL_PERIOD)) {
        diag_.error("unexpected '.' after end of program", currentToken_);
    } else if (!check(TokenType::END_OF_FILE)) {
        diag_.error("expected end of file", currentToken_);
    }
    
    return !diag_.hasErrors();
}

// Recursive Descent Procedures
void Parser::parseProgram() {
    astEnter("Program");
    expect(TokenType::KW_PROGRAM, "expected 'program'");
    expect(TokenType::IDENT, "expected program name");
    std::string progName = previousToken_.literal;
    expect(TokenType::DL_SEMICOLON, "expected ';'");
    
    // Parse program block (code starts at address 0)
    parseBlock(-1);  // -1 indicates main program
    
    if (check(TokenType::DL_PERIOD)) {
        diag_.error("unexpected '.' at end of program", currentToken_);
        advance(); 
    } else if (!check(TokenType::END_OF_FILE)) {
        diag_.error("expected end of file", currentToken_);
    }

    astLeave();
}

void Parser::parseBlock(int procIndex) {
    astEnter("Block");
    
    int savedDataOffset = 4;
    int oldTemp = currentTempOffset_;
    currentTempOffset_ = 3; // Slot 3 is safe in Main (Linkage 0-2)
    int jmpAddr = -1;
    
    jmpAddr = emit(OpCode::JMP, 0, 0);
    symTable_.enterScope();
    
    if (check(TokenType::KW_CONST)) {
        parseConstDecl();
    }
    
    std::vector<int> arrayIndices;
    
    if (check(TokenType::KW_VAR)) {
        parseVarDecl(savedDataOffset, arrayIndices);
    }
    
    while (check(TokenType::KW_PROCEDURE)) {
        parseProcDecl();
        if (check(TokenType::DL_SEMICOLON)) {
            advance();
        }
    }
    
    codeGen_.backpatch(jmpAddr, codeGen_.getNextAddr());
    
    // Update procedure entry address if this is a procedure
    if (procIndex >= 0) {
        symTable_.updateSymbolAddress(procIndex, codeGen_.getNextAddr());
    }
    
    // Generate INT instruction to allocate stack space
    emit(OpCode::INT, 0, savedDataOffset);
    
    // Initialize Arrays (Allocate Heap Memory)
    for (int idx : arrayIndices) {
        Symbol& sym = symTable_.getSymbol(idx);
        // Desciptor: [Address][Size] at sym.address and sym.address+1
        
        // 1. Allocate Heap Memory
        emit(OpCode::LIT, 0, sym.size);
        emit(OpCode::NEW, 0, 0); // Pushes Heap Address
        
        // 2. Store to Descriptor[0] (Address)
        // Note: Using STO 0, addr (current level)
        emit(OpCode::STO, 0, sym.address);
        
        // 3. Store Size to Descriptor[1] (Size)
        emit(OpCode::LIT, 0, sym.size);
        emit(OpCode::STO, 0, sym.address + 1);
    }
    
    // Compound statement (body)
    parseBody();
    
    // Generate return instruction
    emit(OpCode::OPR, 0, static_cast<int>(OprCode::RET));
    
    symTable_.leaveScope();
    
    astLeave();
    currentTempOffset_ = oldTemp;
}

void Parser::parseConstDecl() {
    astEnter("ConstDecl");
    advance();  // Consume 'const'
    
    do {
        expect(TokenType::IDENT, "expected constant name");
        std::string name = previousToken_.literal;
        Token nameToken = previousToken_;
        
        expect(TokenType::OP_ASSIGN, "expected ':='");
        
        // Handle optional sign
        int sign = 1;
        if (match(TokenType::OP_PLUS)) {
            sign = 1;
        } else if (match(TokenType::OP_MINUS)) {
            sign = -1;
        }
        
        expect(TokenType::NUMBER, "expected integer");
        int value = sign * previousToken_.value;
        
        // Register constant
        int idx = symTable_.registerSymbol(name, SymbolKind::CONSTANT, 0);
        if (idx < 0) {
            diag_.error("duplicate identifier: " + name, nameToken);
        } else {
            symTable_.updateSymbolValue(idx, value);
        }
        
    } while (match(TokenType::DL_COMMA));
    
    expect(TokenType::DL_SEMICOLON, "expected ';'");
    
    astLeave();
}

void Parser::parseVarDecl(int& dataOffset, std::vector<int>& arrayIndices) {
    astEnter("VarDecl");
    advance();  // Consume 'var'
    
    do {
        expect(TokenType::IDENT, "expected variable name");
        std::string name = previousToken_.literal;
        Token nameToken = previousToken_;
        
        // Check for Type: var p: pointer; or i: integer;
        if (match(TokenType::DL_COLON)) {
             if (currentToken_.type == TokenType::IDENT && currentToken_.literal == "pointer") {
                 advance(); // consume 'pointer'
                 int idx = symTable_.registerSymbol(name, SymbolKind::POINTER, dataOffset);
                 if (idx < 0) {
                    diag_.error("duplicate identifier: " + name, nameToken);
                 }
                 dataOffset++;
             } else if (currentToken_.type == TokenType::IDENT && currentToken_.literal == "integer") {
                 advance(); // consume 'integer'
                 // Integer is default variable type
                 int idx = symTable_.registerSymbol(name, SymbolKind::VARIABLE, dataOffset);
                 if (idx < 0) {
                    diag_.error("duplicate identifier: " + name, nameToken);
                 }
                 dataOffset++;
             } else {
                 diag_.error("expected type 'pointer' or 'integer'", currentToken_);
             }
        }
        else if (match(TokenType::DL_LBRACKET)) {
            // Array declaration: id[size]
            expect(TokenType::NUMBER, "expected array size");
            int size = previousToken_.value;
            
            if (size <= 0) {
                diag_.error("array size must be positive", previousToken_);
                size = 1;
            }
            
            expect(TokenType::DL_RBRACKET, "expected ']'");
            
            int idx = symTable_.registerSymbol(name, SymbolKind::ARRAY, dataOffset);
            if (idx < 0) {
                diag_.error("duplicate identifier: " + name, nameToken);
            } else {
                symTable_.updateSymbolSize(idx, size);
                arrayIndices.push_back(idx);
            }
            
            // Allocate Descriptor (2 words)
            dataOffset += 2;
        } else {
            // Simple variable declaration
            int idx = symTable_.registerSymbol(name, SymbolKind::VARIABLE, dataOffset);
            if (idx < 0) {
                diag_.error("duplicate identifier: " + name, nameToken);
            }
            dataOffset++;
        }
        
    } while (match(TokenType::DL_COMMA));
    
    expect(TokenType::DL_SEMICOLON, "expected ';'");
    
    astLeave();
}

void Parser::parseProcDecl() {
    astEnter("ProcDecl");
    
    advance();  // Consume 'procedure'
    
    expect(TokenType::IDENT, "expected procedure name");
    std::string name = previousToken_.literal;
    Token nameToken = previousToken_;
    
    // Register procedure (address will be patched later)
    int procIdx = symTable_.registerSymbol(name, SymbolKind::PROCEDURE, 0);
    if (procIdx < 0) {
        diag_.error("duplicate identifier: " + name, nameToken);
        procIdx = symTable_.getTableSize() - 1;  // Use a dummy index
    }
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    // Parse parameters - store names for re-registration in block scope
    std::vector<std::string> paramNames;
    std::vector<int> arrayIndices; // For nested procedures' local arrays
    
    if (!check(TokenType::DL_RPAREN)) {
        do {
            expect(TokenType::IDENT, "expected parameter name");
            paramNames.push_back(previousToken_.literal);
        } while (match(TokenType::DL_COMMA));
    }
    
    int paramCount = static_cast<int>(paramNames.size());
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    // Store parameter count
    if (procIdx >= 0 && procIdx < symTable_.getTableSize()) {
        symTable_.updateSymbolParamCount(procIdx, paramCount);
    }
    
    expect(TokenType::DL_SEMICOLON, "expected ';'");
    
    // JMP over procedure body (will be backpatched)
    int jmpAddr = emit(OpCode::JMP, 0, 0);
    
    // Enter scope for procedure body
    symTable_.enterScope();
    
    // Register parameters in procedure scope
    // Parameters are stored at offset 3, 4, 5, ... (after SL/DL/RA)
    for (int i = 0; i < paramCount; i++) {
        int paramIdx = symTable_.registerSymbol(paramNames[i], SymbolKind::VARIABLE, 3 + i);
        if (paramIdx < 0) {
            diag_.error("duplicate parameter: " + paramNames[i], nameToken);
        }
    }
    
    // Data offset starts after parameters
    // Reserve one slot for Temp (Bounds Check)
    int oldTemp = currentTempOffset_;
    currentTempOffset_ = 3 + paramCount;
    int dataOffset = currentTempOffset_ + 1;
    
    // Constant declaration
    if (check(TokenType::KW_CONST)) {
        parseConstDecl();
    }
    
    // Variable declaration  
    if (check(TokenType::KW_VAR)) {
        parseVarDecl(dataOffset, arrayIndices);
    }
    
    // Nested procedure declarations
    while (check(TokenType::KW_PROCEDURE)) {
        parseProcDecl();
        if (check(TokenType::DL_SEMICOLON)) {
            advance();
        }
    }
    
    // Update procedure entry address and backpatch JMP
    if (procIdx >= 0 && procIdx < symTable_.getTableSize()) {
        symTable_.updateSymbolAddress(procIdx, codeGen_.getNextAddr());
    }
    codeGen_.backpatch(jmpAddr, codeGen_.getNextAddr());
    
    // Generate INT instruction to allocate stack frame
    emit(OpCode::INT, 0, dataOffset);

    // Initialize Arrays (Nested Proc)
    for (int idx : arrayIndices) {
        Symbol& sym = symTable_.getSymbol(idx);
        emit(OpCode::LIT, 0, sym.size);
        emit(OpCode::NEW, 0, 0); 
        emit(OpCode::STO, 0, sym.address);
        emit(OpCode::LIT, 0, sym.size);
        emit(OpCode::STO, 0, sym.address + 1);
    }
    
    // Parse body
    parseBody();
    
    // Generate return
    emit(OpCode::OPR, 0, static_cast<int>(OprCode::RET));
    
    symTable_.leaveScope();
    
    astLeave();
    currentTempOffset_ = oldTemp;
}

void Parser::parseBody() {
    astEnter("Body");
    
    expect(TokenType::KW_BEGIN, "expected 'begin'");
    
    parseStatement();
    
    while (match(TokenType::DL_SEMICOLON)) {
        parseStatement();
    }
    
    expect(TokenType::KW_END, "expected 'end'");
    
    astLeave();
}

void Parser::parseStatement() {
    astEnter("Statement");
    
    if (check(TokenType::IDENT)) {
        advance();
        parseAssignOrArrayAssign();
    } else if (check(TokenType::KW_IF)) {
        parseIfStatement();
    } else if (check(TokenType::KW_WHILE)) {
        parseWhileStatement();
    } else if (check(TokenType::KW_FOR)) {
        parseForStatement();
    } else if (check(TokenType::KW_CALL)) {
        parseCallStatement();
    } else if (check(TokenType::KW_READ)) {
        parseReadStatement();
    } else if (check(TokenType::KW_WRITE)) {
        parseWriteStatement();
    } else if (check(TokenType::KW_NEW)) {
        parseNewStatement();
    } else if (check(TokenType::KW_DELETE)) {
        parseDeleteStatement();
    } else if (check(TokenType::OP_MUL)) {
        // Pointer Assignment: *ptr := val
        advance(); // consume '*'
        
        // The expression following '*' evaluates to the target address
        parseExpression();
        
        expect(TokenType::OP_ASSIGN, "expected ':='");
        
        // The value to assign
        parseExpression();
        
        // Indirect store
        emit(OpCode::STO, 0, 0);
        
    } else if (check(TokenType::KW_BEGIN)) {
        parseBody();
    }
    // Empty statement is also valid (epsilon production)
    
    astLeave();
}

void Parser::parseIfStatement() {
    astEnter("IfStatement");
    
    advance();  // Consume 'if'
    
    parseCondition();
    
    expect(TokenType::KW_THEN, "expected 'then'");
    
    // Conditional jump (jump if false)
    int jpcAddr = emit(OpCode::JPC, 0, 0);
    
    parseStatement();
    
    if (match(TokenType::KW_ELSE)) {
        // Jump over else branch
        int jmpAddr = emit(OpCode::JMP, 0, 0);
        
        // Backpatch JPC to else branch
        codeGen_.backpatch(jpcAddr, codeGen_.getNextAddr());
        
        parseStatement();
        
        // Backpatch JMP to after else
        codeGen_.backpatch(jmpAddr, codeGen_.getNextAddr());
    } else {
        // Backpatch JPC to after then
        codeGen_.backpatch(jpcAddr, codeGen_.getNextAddr());
    }
    
    astLeave();
}

void Parser::parseWhileStatement() {
    astEnter("WhileStatement");
    
    advance();  // Consume 'while'
    
    int loopStart = codeGen_.getNextAddr();
    
    parseCondition();
    
    expect(TokenType::KW_DO, "expected 'do'");
    
    // Conditional jump (exit if false)
    int jpcAddr = emit(OpCode::JPC, 0, 0);
    
    parseStatement();
    
    // Jump back to condition
    emit(OpCode::JMP, 0, loopStart);
    
    // Backpatch exit address
    codeGen_.backpatch(jpcAddr, codeGen_.getNextAddr());
    
    astLeave();
}

void Parser::parseForStatement() {
    astEnter("ForStatement");
    
    advance();  // Consume 'for'
    
    expect(TokenType::IDENT, "expected loop variable");
    std::string varName = previousToken_.literal;
    Token varToken = previousToken_;
    
    // Lookup loop variable
    int varIdx = symTable_.lookup(varName);
    if (varIdx < 0) {
        diag_.error("undefined identifier: " + varName, varToken);
        synchronize();
        astLeave();
        return;
    }
    
    Symbol& varSym = symTable_.getSymbol(varIdx);
    if (varSym.kind != SymbolKind::VARIABLE) {
        diag_.error("loop variable must be a variable", varToken);
    }
    
    expect(TokenType::OP_ASSIGN, "expected ':='");
    
    // Compute initial value
    parseExpression();
    
    // Store to loop variable
    int levelDiff = symTable_.getCurrentLevel() - varSym.level;
    emit(OpCode::STO, levelDiff, varSym.address);
    
    // Check for 'to' or 'downto'
    bool isDownto = false;
    if (match(TokenType::KW_TO)) {
        isDownto = false;
    } else if (match(TokenType::KW_DOWNTO)) {
        isDownto = true;
    } else {
        diag_.error("expected 'to' or 'downto'", currentToken_);
        synchronize();
        astLeave();
        return;
    }
    
    // Record loop start position
    int loopStart = codeGen_.getNextAddr();
    
    // Load loop variable
    emit(OpCode::LOD, levelDiff, varSym.address);
    
    // Compute end value (evaluated each iteration for correctness)
    parseExpression();
    
    // Compare
    if (isDownto) {
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::GEQ));  // >=
    } else {
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::LEQ));  // <=
    }
    
    // Conditional jump (exit if condition false)
    int exitJpc = emit(OpCode::JPC, 0, 0);
    
    expect(TokenType::KW_DO, "expected 'do'");
    
    // Loop body
    parseStatement();
    
    // Step: loop variable +1 or -1
    emit(OpCode::LOD, levelDiff, varSym.address);
    emit(OpCode::LIT, 0, 1);
    if (isDownto) {
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::SUB));
    } else {
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
    }
    emit(OpCode::STO, levelDiff, varSym.address);
    
    // Jump back to loop start
    emit(OpCode::JMP, 0, loopStart);
    
    // Backpatch exit address
    codeGen_.backpatch(exitJpc, codeGen_.getNextAddr());
    
    astLeave();
}

void Parser::parseCallStatement() {
    astEnter("CallStatement");
    
    advance();  // Consume 'call'
    
    expect(TokenType::IDENT, "expected procedure name");
    std::string procName = previousToken_.literal;
    Token procToken = previousToken_;
    
    int idx = symTable_.lookup(procName);
    if (idx < 0) {
        diag_.error("undefined procedure: " + procName, procToken);
        synchronize();
        astLeave();
        return;
    }
    
    Symbol& procSym = symTable_.getSymbol(idx);
    if (procSym.kind != SymbolKind::PROCEDURE) {
        diag_.error("'" + procName + "' is not a procedure", procToken);
        synchronize();
        astLeave();
        return;
    }
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    // Reserve stack frame header space (SL/DL/RA)
    emit(OpCode::INT, 0, 3);
    
    // Parse actual arguments
    int argCount = 0;
    if (!check(TokenType::DL_RPAREN)) {
        do {
            parseExpression();
            argCount++;
        } while (match(TokenType::DL_COMMA));
    }
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    // Check argument count
    if (argCount != procSym.paramCount) {
        diag_.error("argument count mismatch: expected " + std::to_string(procSym.paramCount) + ", got " + std::to_string(argCount), procToken);
    }
    
    // Push argument count (for CAL to calculate new base)
    emit(OpCode::LIT, 0, argCount);
    
    // Generate call instruction
    int levelDiff = symTable_.getCurrentLevel() - procSym.level;
    emit(OpCode::CAL, levelDiff, procSym.address);
    
    astLeave();
}

void Parser::parseReadStatement() {
    astEnter("ReadStatement");
    
    advance();  // Consume 'read'
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    do {
        expect(TokenType::IDENT, "expected variable name");
        std::string name = previousToken_.literal;
        Token nameToken = previousToken_;
        
        int idx = symTable_.lookup(name);
        if (idx < 0) {
            diag_.error("undefined identifier: " + name, nameToken);
            continue;
        }
        
        Symbol& sym = symTable_.getSymbol(idx);
        int levelDiff = symTable_.getCurrentLevel() - sym.level;
        
        // Check for Array Access
        if (check(TokenType::DL_LBRACKET)) {
            if (sym.kind != SymbolKind::ARRAY) {
                diag_.error("'" + name + "' is not an array", nameToken);
            }
            
            parseArrayElementAddress(sym);
            // Stack has Absolute Address
            emit(OpCode::RED, 0, 0); // Indirect Read
            
        } else {
            if (sym.kind != SymbolKind::VARIABLE && sym.kind != SymbolKind::POINTER) {
                diag_.error("'" + name + "' is not a variable", nameToken);
                continue;
            }
            emit(OpCode::RED, levelDiff, sym.address);
        }
        
    } while (match(TokenType::DL_COMMA));
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    astLeave();
}

void Parser::parseWriteStatement() {
    astEnter("WriteStatement");
    
    advance();  // Consume 'write'
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    do {
        parseExpression();
        emit(OpCode::WRT, 0, 0);
    } while (match(TokenType::DL_COMMA));
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    astLeave();
}

void Parser::parseNewStatement() {
    astEnter("NewStatement");
    
    advance();  // Consume 'new'
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    expect(TokenType::IDENT, "expected variable name");
    std::string name = previousToken_.literal;
    Token nameToken = previousToken_;
    
    int idx = symTable_.lookup(name);
    if (idx < 0) {
        diag_.error("undefined identifier: " + name, nameToken);
    }
    
    expect(TokenType::DL_COMMA, "expected ','");
    
    // Compute allocation size
    parseExpression();
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    // Generate heap allocation
    emit(OpCode::NEW, 0, 0);
    
    // Store allocated address to variable
    if (idx >= 0) {
        Symbol& sym = symTable_.getSymbol(idx);
        if (sym.kind != SymbolKind::VARIABLE && sym.kind != SymbolKind::POINTER) {
            diag_.error("'" + name + "' is not a variable or pointer", nameToken);
        } else {
            int levelDiff = symTable_.getCurrentLevel() - sym.level;
            emit(OpCode::STO, levelDiff, sym.address);
        }
    }
    
    astLeave();
}

void Parser::parseDeleteStatement() {
    astEnter("DeleteStatement");
    
    advance();  // Consume 'delete'
    
    expect(TokenType::DL_LPAREN, "expected '('");
    
    expect(TokenType::IDENT, "expected variable name");
    std::string name = previousToken_.literal;
    Token nameToken = previousToken_;
    
    int idx = symTable_.lookup(name);
    if (idx >= 0) {
        Symbol& sym = symTable_.getSymbol(idx);
        if (sym.kind != SymbolKind::VARIABLE && sym.kind != SymbolKind::POINTER) {
            diag_.error("'" + name + "' is not a variable or pointer", nameToken);
        } else {
            int levelDiff = symTable_.getCurrentLevel() - sym.level;
            emit(OpCode::LOD, levelDiff, sym.address);
            emit(OpCode::DEL, 0, 0);
        }
    } else {
        diag_.error("undefined identifier: " + name, nameToken);
    }
    
    expect(TokenType::DL_RPAREN, "expected ')'");
    
    astLeave();
}

void Parser::parseAssignOrArrayAssign() {
    astEnter("AssignStatement");
    
    std::string name = previousToken_.literal;
    Token idToken = previousToken_;
    
    int idx = symTable_.lookup(name);
    if (idx < 0) {
        diag_.error("undefined identifier: " + name, idToken);
        synchronize();
        astLeave();
        return;
    }
    
    Symbol& sym = symTable_.getSymbol(idx);
    int levelDiff = symTable_.getCurrentLevel() - sym.level;
    
    // Check for Array Access
    if (check(TokenType::DL_LBRACKET)) {
        // Array Assignment: arr[i] := expr
        parseArrayElementAddress(sym);
        // Stack Top is now Absolute Address of element
        
        expect(TokenType::OP_ASSIGN, "expected ':='");
        
        parseExpression();
        // Stack: [Address, Value] (from bottom to top)
        
        // Indirect Store
        emit(OpCode::STO, 0, 0);
        
    } else {
        // Simple Assignment: x := expr
        // Valid for Variable and Pointer
        if (sym.kind != SymbolKind::VARIABLE && sym.kind != SymbolKind::POINTER) {
             diag_.error("cannot assign to constant, procedure, or array (without index)", idToken);
        }
        
        expect(TokenType::OP_ASSIGN, "expected ':='");
        
        parseExpression();
        
        emit(OpCode::STO, levelDiff, sym.address);
    }
    
    astLeave();
}

// Helper: Parse Array Element Address (Pushes Absolute Address)
void Parser::parseArrayElementAddress(Symbol& sym) {
    if (sym.kind != SymbolKind::ARRAY && sym.kind != SymbolKind::POINTER && sym.kind != SymbolKind::VARIABLE) {
        diag_.error("identifier cannot be indexed", currentToken_);
    }
    
    int levelDiff = symTable_.getCurrentLevel() - sym.level;
    
    // 1. Load Heap Address 
    // For ARRAY: stored at sym.address (Descriptor[0])
    // For POINTER/VAR: stored at sym.address (Value)
    emit(OpCode::LOD, levelDiff, sym.address);
    
    expect(TokenType::DL_LBRACKET, "expected '['");
    
    // 2. Parse Index
    parseExpression();
    
    expect(TokenType::DL_RBRACKET, "expected ']'");
    
    // Bounds Check only for declared arrays
    if (sym.kind == SymbolKind::ARRAY) {
        // A. Bounds Check Logic (Using reserved stack slot as Temp)
        // Store Index to Temp
        emit(OpCode::STO, 0, currentTempOffset_);
        
        // Check Index >= 0
        emit(OpCode::LOD, 0, currentTempOffset_);
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::GEQ)); 
        int jpcFail1 = emit(OpCode::JPC, 0, 0); // Jump if false (Index < 0)
        
        // Check Index < Size
        emit(OpCode::LOD, 0, currentTempOffset_);
        emit(OpCode::LOD, levelDiff, sym.address + 1); // Load Size from Descriptor[1]
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::LSS)); 
        int jpcFail2 = emit(OpCode::JPC, 0, 0); // Jump if false (Index >= Size)
        
        // Restore Index
        emit(OpCode::LOD, 0, currentTempOffset_);
        
        // Error Handling Block Generation
        // (Pre-generate jump over error)
        // Actually, we need to construct the error handling differently to avoid executing it
        // We need to jump OVER the error block if checks passed.
        // Wait, current logic was:
        // if (fail) decode error...
        // My previous code:
        // Jump if Fail -> Error Block
        // But the JPC jumps if FALSE (0).
        // GEQ returns 1 if true. JPC jumps if 0.
        // So JPC jumps if Index < 0. Correct.
        
        // Backpatching later:
        // We need to JUMP over the following error generation if we didn't fail?
        // No, the previous code structure was:
        // [Check 1] -> JPC(Fail1)
        // [Check 2] -> JPC(Fail2)
        // [Restore Index]
        // [Add Base]
        // JMP(Success) -> Skip Error
        // Fail1/Fail2: [Error Code]
        // Success: ...
        
        // Re-implementing correctly:
        // (Code from before)
        
        // 3. Compute Absolute Address (HeapAddr + Index)
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
        
        int jumpOverError = emit(OpCode::JMP, 0, 0);
        
        // Error Block
        int errorAddr = codeGen_.getNextAddr();
        codeGen_.backpatch(jpcFail1, errorAddr);
        codeGen_.backpatch(jpcFail2, errorAddr);
        
        // Runtime Error
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::LIT, 0, 0);
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::DIV)); // Division by zero trigger
        
        codeGen_.backpatch(jumpOverError, codeGen_.getNextAddr());
    } else {
        // No bounds check for Pointers
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
    }
}

void Parser::parseCondition() {
    astEnter("Condition");
    
    if (match(TokenType::KW_ODD)) {
        parseExpression();
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::ODD));
    } else {
        parseExpression();
        
        OprCode oprCode;
        if (match(TokenType::OP_EQ)) {
            oprCode = OprCode::EQL;
        } else if (match(TokenType::OP_NE)) {
            oprCode = OprCode::NEQ;
        } else if (match(TokenType::OP_LT)) {
            oprCode = OprCode::LSS;
        } else if (match(TokenType::OP_LE)) {
            oprCode = OprCode::LEQ;
        } else if (match(TokenType::OP_GT)) {
            oprCode = OprCode::GTR;
        } else if (match(TokenType::OP_GE)) {
            oprCode = OprCode::GEQ;
        } else {
            diag_.error("expected relational operator", currentToken_);
            astLeave();
            return;
        }
        
        parseExpression();
        emit(OpCode::OPR, 0, static_cast<int>(oprCode));
    }
    
    astLeave();
}

void Parser::parseExpression() {
    astEnter("Expression");
    
    // Optional leading sign
    bool negate = false;
    if (match(TokenType::OP_PLUS)) {
        // No operation needed
    } else if (match(TokenType::OP_MINUS)) {
        negate = true;
    }
    
    parseTerm();
    
    if (negate) {
        emit(OpCode::OPR, 0, static_cast<int>(OprCode::NEG));
    }
    
    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
        TokenType op = currentToken_.type;
        advance();
        
        parseTerm();
        
        if (op == TokenType::OP_PLUS) {
            emit(OpCode::OPR, 0, static_cast<int>(OprCode::ADD));
        } else {
            emit(OpCode::OPR, 0, static_cast<int>(OprCode::SUB));
        }
    }
    
    astLeave();
}

void Parser::parseTerm() {
    astEnter("Term");
    
    parseFactor();
    
    while (check(TokenType::OP_MUL) || check(TokenType::OP_DIV) || 
           check(TokenType::KW_MOD)) {
        TokenType op = currentToken_.type;
        advance();
        
        parseFactor();
        
        if (op == TokenType::OP_MUL) {
            emit(OpCode::OPR, 0, static_cast<int>(OprCode::MUL));
        } else if (op == TokenType::OP_DIV) {
            emit(OpCode::OPR, 0, static_cast<int>(OprCode::DIV));
        } else {
            emit(OpCode::OPR, 0, static_cast<int>(OprCode::MOD));
        }
    }
    
    astLeave();
}

void Parser::parseFactor() {
    astEnter("Factor");
    
    // 1. Dereference (*p)
    if (currentToken_.type == TokenType::OP_MUL) { // '*'
        advance();
        parseFactor();
        emit(OpCode::LOD, 0, 0); // Indirect Load
    }
    // 2. Address-of (&x)
    else if (currentToken_.type == TokenType::OP_ADDR) { // '&'
        advance();
        expect(TokenType::IDENT, "expected identifier after '&'");
        std::string name = previousToken_.literal;
        Token nameToken = previousToken_;
        
        int idx = symTable_.lookup(name);
        if (idx < 0) {
            diag_.error("undefined identifier: " + name, nameToken);
            astLeave(); return;
        }
        Symbol& sym = symTable_.getSymbol(idx);
        int levelDiff = symTable_.getCurrentLevel() - sym.level;
        
        if (check(TokenType::DL_LBRACKET)) {
             // &arr[i]
             // Delegated to parseArrayElementAddress for type checking
             parseArrayElementAddress(sym);
             // Stack has Abs Address. Do not load.
        } else {
             // &var or &arr
             if (sym.kind == SymbolKind::VARIABLE || sym.kind == SymbolKind::POINTER) {
                 emit(OpCode::LAD, levelDiff, sym.address);
             } else if (sym.kind == SymbolKind::ARRAY) {
                 // Array name decay to pointer (Heap Address)
                 emit(OpCode::LOD, levelDiff, sym.address);
             } else {
                 diag_.error("cannot take address of this symbol", nameToken);
             }
        }
    }
    // 3. Identifier
    else if (match(TokenType::IDENT)) {
        std::string name = previousToken_.literal;
        Token idToken = previousToken_;
        
        int idx = symTable_.lookup(name);
        if (idx < 0) {
            diag_.error("undefined identifier: " + name, idToken);
            astLeave(); return;
        }
        
        Symbol& sym = symTable_.getSymbol(idx);
        int levelDiff = symTable_.getCurrentLevel() - sym.level;
        
        if (check(TokenType::DL_LBRACKET)) {
            // Array Access: arr[i]
            // Delegated to parseArrayElementAddress for type checking
            parseArrayElementAddress(sym);
            emit(OpCode::LOD, 0, 0); // Indirect Load
        } else {
            // Simple Var/Const/Pointer
            if (sym.kind == SymbolKind::CONSTANT) {
                emit(OpCode::LIT, 0, sym.value);
            } else if (sym.kind == SymbolKind::VARIABLE || sym.kind == SymbolKind::POINTER) {
                emit(OpCode::LOD, levelDiff, sym.address);
            } else if (sym.kind == SymbolKind::ARRAY) {
                diag_.error("cannot use array '" + name + "' without subscript", idToken);
            } else {
                diag_.error("invalid identifier type", idToken);
            }
        }
    }
    // 4. Number
    else if (match(TokenType::NUMBER)) {
        emit(OpCode::LIT, 0, previousToken_.value);
    }
    // 5. Parentheses
    else if (match(TokenType::DL_LPAREN)) {
        parseExpression();
        expect(TokenType::DL_RPAREN, "expected ')'");
    }
    else {
        diag_.error("unexpected token in expression", currentToken_);
        advance(); 
    }
    
    astLeave();
}

} // namespace pl0
