#ifndef PL0_LEXER_H
#define PL0_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

namespace pl0 {

class DiagnosticsEngine;

// Lexer class
// Implements double-buffered input with sentinel mechanism
class Lexer {
public:
    // Buffer configuration
    static constexpr size_t BUFFER_SIZE = 4096;
    static constexpr char SENTINEL = '\0';

    // Construct from source string
    Lexer(const std::string& source, DiagnosticsEngine& diag);
    
    ~Lexer() = default;

    // Reset lexer to beginning of source
    void reset();

    // Get next token
    Token nextToken();

    // Peek next token (without consuming)
    Token peekToken();

    // Get all tokens (for --tokens output)
    std::vector<Token> tokenize();

private:
    // Look at current character (no advance)
    char peek();
    
    // Look at next character (no advance)
    char peekNext();
    
    // Advance and return current character
    char advance();
    
    // Check if at end
    bool isAtEnd() const;
    
    // Mark current position as lexeme start
    void markLexemeStart();
    
    // Get lexeme from start to current position
    std::string getLexeme() const;

    void skipWhitespace();
    void skipWhitespaceAndComments();
    bool skipLineComment();         // Handle //
    bool skipBlockComment();        // Handle /* */
    bool skipPascalComment();       // Handle { }
    
    Token scanIdentifierOrKeyword();
    Token scanNumber();
    Token scanOperatorOrDelimiter();
    Token scanUnknown();
    
    // UTF-8 Handling 
    
    int getUtf8CharLen(unsigned char c) const;
    std::string readUtf8Char();
    int getUtf8StringLen(const std::string& s) const;
    bool isValidPunctStart(char c) const;

    // Token Creation 
    
    Token makeToken(TokenType type);
    Token makeToken(TokenType type, const std::string& literal);
    Token makeToken(TokenType type, int value);

    // Double Buffering Internals
    
    // Switch to next buffer and load data
    void loadNextBuffer();
    
    // Data Members 
    
    std::string source_;        // Acts as the "file" content
    size_t sourcePtr_;          // Current "file" read position
    
    char buffers_[2][BUFFER_SIZE + 1]; // buffers[0] and buffers[1], +1 for sentinel
    int currentBufferIdx_;      // 0 or 1
    
    char* lexemeBegin_;         // Pointer to start of lexeme in current buffer
    char* forward_;             // Scan pointer
    
    std::string partialLexeme_; // Holds first part of lexeme if it spans buffers
    bool isEof_;                // flag to indicate real EOF encountered
    
    int line_;                  
    int column_;               
    int tokenStartLine_;       
    int tokenStartColumn_;   
    
    DiagnosticsEngine& diag_;
    
    // Token buffer for peek
    Token bufferedToken_;
    bool hasBuffered_;

    static const std::unordered_map<std::string, TokenType> keywords_;
};

} // namespace pl0

#endif // PL0_LEXER_H
