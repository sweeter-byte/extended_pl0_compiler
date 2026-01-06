#ifndef PL0_TOKEN_H
#define PL0_TOKEN_H

#include <string>

namespace pl0 {

// Token type enumeration
enum class TokenType {
    // Special markers
    END_OF_FILE,        // End of file
    UNKNOWN,            // Unknown/illegal character sequence

    // Identifiers and literals
    IDENT,              // Identifier
    NUMBER,             // Integer literal

    // Keywords
    KW_PROGRAM,         // program
    KW_CONST,           // const
    KW_VAR,             // var
    KW_PROCEDURE,       // procedure
    KW_BEGIN,           // begin
    KW_END,             // end
    KW_IF,              // if
    KW_THEN,            // then
    KW_ELSE,            // else
    KW_WHILE,           // while
    KW_DO,              // do
    KW_FOR,             // for
    KW_TO,              // to
    KW_DOWNTO,          // downto
    KW_CALL,            // call
    KW_READ,            // read
    KW_WRITE,           // write
    KW_ODD,             // odd
    KW_MOD,             // mod
    KW_NEW,             // new
    KW_DELETE,          // delete

    // Operators
    OP_PLUS,            // +
    OP_MINUS,           // -
    OP_MUL,             // *
    OP_DIV,             // /
    OP_EQ,              // =
    OP_NE,              // <>
    OP_LT,              // <
    OP_LE,              // <=
    OP_GT,              // >
    OP_GE,              // >=
    OP_ASSIGN,          // :=
    OP_ADDR,            // &

    // Delimiters
    DL_LPAREN,          // (
    DL_RPAREN,          // )
    DL_LBRACKET,        // [
    DL_RBRACKET,        // ]
    DL_COMMA,           // ,
    DL_SEMICOLON,       // ;
    DL_PERIOD,          // .
    DL_COLON            // :
};

// Token structure
struct Token {
    TokenType type;         // Token type
    std::string literal;    // Lexeme (UTF-8)
    int value;              // Numeric value (only valid for NUMBER type)
    int line;               // Line number (1-based)
    int column;             // Column number (1-based, character count)
    int length;             // Token length (character count, for error indication)

    Token() : type(TokenType::END_OF_FILE), value(0), line(0), column(0), length(0) {}
    
    Token(TokenType t, const std::string& lit, int ln, int col, int len)
        : type(t), literal(lit), value(0), line(ln), column(col), length(len) {}
};

const char* tokenTypeToString(TokenType type);

} // namespace pl0

#endif // PL0_TOKEN_H
