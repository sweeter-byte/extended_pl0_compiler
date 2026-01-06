#include "Lexer.h"
#include "Diagnostics.h"
#include "Common.h"
#include <cctype>
#include <climits>
#include <unordered_map>
#include <cstring>
#include <algorithm>

namespace pl0 {
const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"program", TokenType::KW_PROGRAM},
    {"const", TokenType::KW_CONST},
    {"var", TokenType::KW_VAR},
    {"procedure", TokenType::KW_PROCEDURE},
    {"begin", TokenType::KW_BEGIN},
    {"end", TokenType::KW_END},
    {"if", TokenType::KW_IF},
    {"then", TokenType::KW_THEN},
    {"else", TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"do", TokenType::KW_DO},
    {"for", TokenType::KW_FOR},
    {"to", TokenType::KW_TO},
    {"downto", TokenType::KW_DOWNTO},
    {"call", TokenType::KW_CALL},
    {"read", TokenType::KW_READ},
    {"write", TokenType::KW_WRITE},
    {"odd", TokenType::KW_ODD},
    {"mod", TokenType::KW_MOD},
    {"new", TokenType::KW_NEW},
    {"delete", TokenType::KW_DELETE}
};

Lexer::Lexer(const std::string& source, DiagnosticsEngine& diag)
    : source_(source), sourcePtr_(0), 
      currentBufferIdx_(1), // Start at 1 so first load switches to 0
      hasBuffered_(false),
      line_(1), column_(1), tokenStartLine_(1), tokenStartColumn_(1),
      diag_(diag) {
    
    // Initialize pointers to trigger initial load
    forward_ = buffers_[1] + BUFFER_SIZE;
    lexemeBegin_ = buffers_[1];
    
    // Load first buffer
    loadNextBuffer();
    
    // Set lexeme start to beginning of first buffer
    markLexemeStart();
}

void Lexer::reset() {
    sourcePtr_ = 0;
    currentBufferIdx_ = 1;
    forward_ = buffers_[1] + BUFFER_SIZE;
    lexemeBegin_ = buffers_[1];
    line_ = 1;
    column_ = 1;
    partialLexeme_.clear();
    hasBuffered_ = false;
    
    loadNextBuffer();
    markLexemeStart();
}

// Double Buffering Logic

void Lexer::loadNextBuffer() {
    // If there is an active lexeme part in the current buffer, save it
    if (lexemeBegin_ < forward_) {
        partialLexeme_.append(lexemeBegin_, forward_);
    } else if (lexemeBegin_ >= buffers_[currentBufferIdx_] && 
               lexemeBegin_ < buffers_[currentBufferIdx_] + BUFFER_SIZE) {
        // Cases where lexemeBegin_ is in the buffer but forward_ is at the end sentinel
        partialLexeme_.append(lexemeBegin_, forward_);
    }
    
    // Switch buffer
    currentBufferIdx_ = 1 - currentBufferIdx_;
    char* buffer = buffers_[currentBufferIdx_];
    
    // Read from source
    size_t remaining = source_.length() - sourcePtr_;
    size_t toRead = std::min(remaining, BUFFER_SIZE);
    
    if (toRead > 0) {
        std::memcpy(buffer, source_.data() + sourcePtr_, toRead);
        sourcePtr_ += toRead;
    }
    
    // Place sentinel
    buffer[toRead] = SENTINEL;
    isEof_ = (toRead == 0);
    
    // Reset pointers to start of new buffer
    forward_ = buffer;
    lexemeBegin_ = buffer;
}

char Lexer::peek() {
    char c = *forward_;
    
    if (c == SENTINEL) {
        // Check if we are at the physical end of the buffer
        if (forward_ == buffers_[currentBufferIdx_] + BUFFER_SIZE) {
            loadNextBuffer();
            return peek(); // Retry in new buffer
        }
        // Real EOF
        return '\0';
    }
    
    return c;
}

char Lexer::peekNext() {
    char* p = forward_ + 1;
    char c = *p;
    
    if (c == SENTINEL) {
        if (p == buffers_[currentBufferIdx_] + BUFFER_SIZE) {
            if (sourcePtr_ < source_.length()) {
                return source_[sourcePtr_];
            }
            return '\0';
        }
    }
    return c;
}

char Lexer::advance() {
    char c = *forward_;
    
    if (c == SENTINEL) {
        if (forward_ == buffers_[currentBufferIdx_] + BUFFER_SIZE) {
            loadNextBuffer();
            return advance();
        }
        // Real EOF
        return '\0';
    }
    
    forward_++;
    
    // Update position tracking
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    
    return c;
}

bool Lexer::isAtEnd() const {
    return const_cast<Lexer*>(this)->peek() == '\0';
}

void Lexer::markLexemeStart() {
    lexemeBegin_ = forward_;
    partialLexeme_.clear();
    tokenStartLine_ = line_;
    tokenStartColumn_ = column_;
}

std::string Lexer::getLexeme() const {
    std::string s = partialLexeme_;
    if (forward_ > lexemeBegin_) {
        s.append(lexemeBegin_, forward_ - lexemeBegin_);
    }
    return s;
}

// Whitespace and Comments 

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        skipWhitespace();
        
        if (peek() == '/' && peekNext() == '/') {
            skipLineComment();
        } else if (peek() == '/' && peekNext() == '*') {
            skipBlockComment();
        } else if (peek() == '{') {
            skipPascalComment();
        } else {
            break;
        }
    }
}

bool Lexer::skipLineComment() {
    // Skip //
    advance();
    advance();
    
    // Skip until end of line
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
    
    return true;
}

bool Lexer::skipBlockComment() {
    int startLine = line_;
    int startCol = column_;
    
    // Skip /*
    advance();
    advance();
    
    while (!isAtEnd()) {
        if (peek() == '*' && peekNext() == '/') {
            advance();  // Skip *
            advance();  // Skip /
            return true;
        }
        advance();
    }
    
    // Unclosed block comment
    diag_.error("unclosed block comment", startLine, startCol, 2);
    return false;
}

bool Lexer::skipPascalComment() {
    int startLine = line_;
    int startCol = column_;
    
    // Skip {
    advance();
    
    while (!isAtEnd() && peek() != '}') {
        advance();
    }
    
    if (isAtEnd()) {
        diag_.error("unclosed comment", startLine, startCol, 1);
        return false;
    }
    
    advance();  // Skip }
    return true;
}

// UTF-8 Handling 

int Lexer::getUtf8CharLen(unsigned char c) const {
    return utf8CharLen(c);
}

std::string Lexer::readUtf8Char() {
    std::string result;
    
    if (isAtEnd()) {
        return result;
    }
    
    unsigned char c = static_cast<unsigned char>(peek());
    int len = getUtf8CharLen(c);
    
    for (int i = 0; i < len && !isAtEnd(); i++) {
        result += advance();
    }
    
    return result;
}

int Lexer::getUtf8StringLen(const std::string& s) const {
    return utf8StringLen(s);
}

bool Lexer::isValidPunctStart(char c) const {
    // Valid punctuation characters in PL/0
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '=' || c == '<' || c == '>' || c == ':' ||
           c == '(' || c == ')' || c == '[' || c == ']' ||
           c == ',' || c == ';' || c == '.' || c == '&';
}

// Token Creation 

Token Lexer::makeToken(TokenType type) {
    std::string lexeme = getLexeme();
    int len = getUtf8StringLen(lexeme);
    return Token(type, lexeme, tokenStartLine_, tokenStartColumn_, len);
}

Token Lexer::makeToken(TokenType type, const std::string& literal) {
    int len = getUtf8StringLen(literal);
    return Token(type, literal, tokenStartLine_, tokenStartColumn_, len);
}

Token Lexer::makeToken(TokenType type, int value) {
    Token tok = makeToken(type);
    tok.value = value;
    return tok;
}

// Scanning Methods 

Token Lexer::scanIdentifierOrKeyword() {
    markLexemeStart();
    
    // First character is already verified as alpha
    advance();
    
    // Continue with alphanumeric only (PL/0 strict)
    while (!isAtEnd() && std::isalnum(peek())) {
        advance();
    }
    
    std::string lexeme = getLexeme();
    
    // Check if it's a keyword
    auto it = keywords_.find(lexeme);
    if (it != keywords_.end()) {
        return makeToken(it->second);
    }
    
    return makeToken(TokenType::IDENT);
}

Token Lexer::scanNumber() {
    markLexemeStart();
    
    while (!isAtEnd() && std::isdigit(peek())) {
        advance();
    }
    
    std::string lexeme = getLexeme();
    
    // Check for overflow
    long long value = 0;
    try {
        value = std::stoll(lexeme);
        if (value > INT_MAX) {
            diag_.error("integer literal overflow", tokenStartLine_, tokenStartColumn_, static_cast<int>(lexeme.size()));
            value = 0;
        }
    } catch (...) {
        diag_.error("invalid integer literal", tokenStartLine_, tokenStartColumn_, static_cast<int>(lexeme.size()));
        value = 0;
    }
    
    return makeToken(TokenType::NUMBER, static_cast<int>(value));
}

Token Lexer::scanOperatorOrDelimiter() {
    markLexemeStart();
    
    char c = advance();
    
    switch (c) {
        case '+': return makeToken(TokenType::OP_PLUS);
        case '-': return makeToken(TokenType::OP_MINUS);
        case '*': return makeToken(TokenType::OP_MUL);
        case '/': return makeToken(TokenType::OP_DIV);
        case '=': return makeToken(TokenType::OP_EQ);
        
        case '<':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::OP_LE);
            }
            if (peek() == '>') {
                advance();
                return makeToken(TokenType::OP_NE);
            }
            return makeToken(TokenType::OP_LT);
        
        case '>':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::OP_GE);
            }
            return makeToken(TokenType::OP_GT);
        
        case ':':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::OP_ASSIGN);
            }
            return makeToken(TokenType::DL_COLON);

        case '&': return makeToken(TokenType::OP_ADDR);
        
        case '(': return makeToken(TokenType::DL_LPAREN);
        case ')': return makeToken(TokenType::DL_RPAREN);
        case '[': return makeToken(TokenType::DL_LBRACKET);
        case ']': return makeToken(TokenType::DL_RBRACKET);
        case ',': return makeToken(TokenType::DL_COMMA);
        case ';': return makeToken(TokenType::DL_SEMICOLON);
        case '.': return makeToken(TokenType::DL_PERIOD);
        
        default:
            // Should not reach here if called correctly
            return makeToken(TokenType::UNKNOWN);
    }
}

Token Lexer::scanUnknown() {
    markLexemeStart();
    
    std::string unknown;
    int startLine = line_;
    int startCol = column_;
    
    // Greedy match: collect all consecutive illegal characters
    while (!isAtEnd()) {
        char c = peek();
        
        // Stop at valid character categories
        if (std::isalnum(c) || std::isspace(c)) {
            break;
        }
        if (std::ispunct(c) && isValidPunctStart(c)) {
            break;
        }
        
        // Read one complete UTF-8 character
        unknown += readUtf8Char();
    }
    
    // Report single error for merged illegal characters
    int charLen = getUtf8StringLen(unknown);
    diag_.error("illegal character sequence: '" + unknown + "'", startLine, startCol, charLen);
    
    return Token(TokenType::UNKNOWN, unknown, startLine, startCol, charLen);
}

Token Lexer::nextToken() {
    // Return buffered token if available
    if (hasBuffered_) {
        hasBuffered_ = false;
        return bufferedToken_;
    }
    
    skipWhitespaceAndComments();
    
    if (isAtEnd()) {
        return makeToken(TokenType::END_OF_FILE, "");
    }
    
    markLexemeStart();
    char c = peek();
    
    // Identifier or keyword
    if (std::isalpha(c)) {
        return scanIdentifierOrKeyword();
    }
    
    // Number
    if (std::isdigit(c)) {
        return scanNumber();
    }
    
    // Operator or delimiter
    if (isValidPunctStart(c)) {
        return scanOperatorOrDelimiter();
    }
    
    // Illegal character (including UTF-8 characters like Chinese)
    return scanUnknown();
}

Token Lexer::peekToken() {
    if (!hasBuffered_) {
        bufferedToken_ = nextToken();
        hasBuffered_ = true;
    }
    return bufferedToken_;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    
    // Reset state
    reset();
    
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        
        if (tok.type == TokenType::END_OF_FILE) {
            break;
        }
    }
    
    return tokens;
}

} // namespace pl0
