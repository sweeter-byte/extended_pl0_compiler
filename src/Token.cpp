#include "Token.h"

namespace pl0 {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE:    return "EOF";
        case TokenType::UNKNOWN:        return "UNKNOWN";
        case TokenType::IDENT:          return "IDENT";
        case TokenType::NUMBER:         return "NUMBER";
        case TokenType::KW_PROGRAM:     return "PROGRAM";
        case TokenType::KW_CONST:       return "CONST";
        case TokenType::KW_VAR:         return "VAR";
        case TokenType::KW_PROCEDURE:   return "PROCEDURE";
        case TokenType::KW_BEGIN:       return "BEGIN";
        case TokenType::KW_END:         return "END";
        case TokenType::KW_IF:          return "IF";
        case TokenType::KW_THEN:        return "THEN";
        case TokenType::KW_ELSE:        return "ELSE";
        case TokenType::KW_WHILE:       return "WHILE";
        case TokenType::KW_DO:          return "DO";
        case TokenType::KW_FOR:         return "FOR";
        case TokenType::KW_TO:          return "TO";
        case TokenType::KW_DOWNTO:      return "DOWNTO";
        case TokenType::KW_CALL:        return "CALL";
        case TokenType::KW_READ:        return "READ";
        case TokenType::KW_WRITE:       return "WRITE";
        case TokenType::KW_ODD:         return "ODD";
        case TokenType::KW_MOD:         return "MOD";
        case TokenType::KW_NEW:         return "NEW";
        case TokenType::KW_DELETE:      return "DELETE";
        case TokenType::OP_PLUS:        return "PLUS";
        case TokenType::OP_MINUS:       return "MINUS";
        case TokenType::OP_MUL:         return "MUL";
        case TokenType::OP_DIV:         return "DIV";
        case TokenType::OP_EQ:          return "EQ";
        case TokenType::OP_NE:          return "NE";
        case TokenType::OP_LT:          return "LT";
        case TokenType::OP_LE:          return "LE";
        case TokenType::OP_GT:          return "GT";
        case TokenType::OP_GE:          return "GE";
        case TokenType::OP_ASSIGN:      return "ASSIGN";
        case TokenType::DL_LPAREN:      return "LPAREN";
        case TokenType::DL_RPAREN:      return "RPAREN";
        case TokenType::DL_LBRACKET:    return "LBRACKET";
        case TokenType::DL_RBRACKET:    return "RBRACKET";
        case TokenType::DL_COMMA:       return "COMMA";
        case TokenType::DL_SEMICOLON:   return "SEMICOLON";
        case TokenType::DL_PERIOD:      return "PERIOD";
        default:                        return "???";
    }
}

} // namespace pl0
