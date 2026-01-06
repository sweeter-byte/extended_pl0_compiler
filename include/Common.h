#ifndef PL0_COMMON_H
#define PL0_COMMON_H

#include <string>
#include <cstdint>

namespace pl0 {
namespace Color {
    inline const char* Reset   = "\033[0m";
    inline const char* Red     = "\033[31m";
    inline const char* Green   = "\033[32m";
    inline const char* Yellow  = "\033[33m";
    inline const char* Blue    = "\033[34m";
    inline const char* Magenta = "\033[35m";
    inline const char* Cyan    = "\033[36m";
    inline const char* White   = "\033[37m";
    inline const char* Bold    = "\033[1m";
}

bool isTerminal();

constexpr int MAX_IDENT_LEN = 64;
constexpr int MAX_NUMBER_LEN = 10;
constexpr int MAX_NUMBER_VALUE = 2147483647;
constexpr int DEFAULT_STORE_SIZE = 10000;

int utf8CharLen(unsigned char c);
int utf8StringLen(const std::string& s);
std::string utf8Substr(const std::string& s, int start, int len);

} // namespace pl0

#endif // PL0_COMMON_H
