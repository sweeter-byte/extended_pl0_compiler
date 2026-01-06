#ifndef PL0_SOURCE_MANAGER_H
#define PL0_SOURCE_MANAGER_H

#include <string>
#include <vector>

namespace pl0 {

// Source code manager
// Handles source file loading and line-based access
class SourceManager {
public:
    SourceManager() = default;

    // Load source file, returns true on success
    bool loadFile(const std::string& filename);

    // Load source from string (for testing)
    void loadString(const std::string& source, const std::string& filename = "<string>");

    // Get line content (line number is 1-based)
    std::string getLine(int lineNum) const;

    // Get total line count
    int getLineCount() const { return static_cast<int>(lines_.size()); }

    // Get filename
    const std::string& getFilename() const { return filename_; }

    // Get entire source content
    const std::string& getSource() const { return source_; }

private:
    // Split source into lines
    void splitLines();

    std::string filename_;
    std::string source_;            // Complete source code
    std::vector<std::string> lines_; // Split by lines
};

} // namespace pl0

#endif // PL0_SOURCE_MANAGER_H
