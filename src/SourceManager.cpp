#include "SourceManager.h"
#include <fstream>
#include <sstream>

namespace pl0 {

bool SourceManager::loadFile(const std::string& filename) {
    filename_ = filename;
    
    // Read file in binary mode to preserve original bytes
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    source_ = buffer.str();
    
    splitLines();
    return true;
}

void SourceManager::loadString(const std::string& source, const std::string& filename) {
    filename_ = filename;
    source_ = source;
    splitLines();
}

void SourceManager::splitLines() {
    lines_.clear();
    
    std::string line;
    std::istringstream stream(source_);
    
    while (std::getline(stream, line)) {
        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines_.push_back(line);
    }
    
    // Handle case where source doesn't end with newline
    if (!source_.empty() && source_.back() != '\n' && lines_.empty()) {
        lines_.push_back(source_);
    }
}

std::string SourceManager::getLine(int lineNum) const {
    if (lineNum < 1 || lineNum > static_cast<int>(lines_.size())) {
        return "";
    }
    return lines_[lineNum - 1];
}

} // namespace pl0
