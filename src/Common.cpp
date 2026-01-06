#include "Common.h"
#include <cstdio>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace pl0 {

bool isTerminal() {
    return isatty(fileno(stdout)) != 0;
}

// Get UTF-8 character byte length from first byte
int utf8CharLen(unsigned char c) {
    if ((c & 0x80) == 0) return 1;          // 0xxxxxxx - ASCII
    if ((c & 0xE0) == 0xC0) return 2;       // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;       // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;       // 11110xxx
    return 1;  // Invalid, treat as single byte
}

// Get UTF-8 string character count
int utf8StringLen(const std::string& s) {
    int count = 0;
    size_t i = 0;
    while (i < s.size()) {
        i += utf8CharLen(static_cast<unsigned char>(s[i]));
        count++;
    }
    return count;
}

// Get UTF-8 substring by character indices
std::string utf8Substr(const std::string& s, int start, int len) {
    std::string result;
    size_t i = 0;
    int charCount = 0;
    
    // Skip to start position
    while (i < s.size() && charCount < start) {
        i += utf8CharLen(static_cast<unsigned char>(s[i]));
        charCount++;
    }
    
    // Extract len characters
    size_t startByte = i;
    int extracted = 0;
    while (i < s.size() && extracted < len) {
        i += utf8CharLen(static_cast<unsigned char>(s[i]));
        extracted++;
    }
    
    return s.substr(startByte, i - startByte);
}

} // namespace pl0
