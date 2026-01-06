#ifndef PL0_DIAGNOSTICS_H
#define PL0_DIAGNOSTICS_H

#include <string>
#include <vector>
#include "Token.h"

namespace pl0 {
class SourceManager;

enum class DiagLevel {
    ERROR,      // Error (red)
    WARNING,    // Warning (yellow)
    NOTE        // Note (cyan)
};

// Single diagnostic record
struct Diagnostic {
    DiagLevel level;
    std::string message;
    int line;
    int column;
    int length;     // For generating ~~~ underline

    Diagnostic(DiagLevel lv, const std::string& msg, int ln, int col, int len)
        : level(lv), message(msg), line(ln), column(col), length(len) {}
};

// Diagnostics engine (Clang-style output)
class DiagnosticsEngine {
public:
    explicit DiagnosticsEngine(const SourceManager& srcMgr);

    // Report error
    void error(const std::string& msg, int line, int col, int len = 1);
    void error(const std::string& msg, const Token& tok);

    // Report warning
    void warning(const std::string& msg, int line, int col, int len = 1);
    void warning(const std::string& msg, const Token& tok);

    // Report note
    void note(const std::string& msg, int line, int col, int len = 1);

    // Query status
    bool hasErrors() const { return errorCount_ > 0; }
    int getErrorCount() const { return errorCount_; }
    int getWarningCount() const { return warningCount_; }

    // Set maximum error count (compilation stops after exceeding)
    void setMaxErrors(int max) { maxErrors_ = max; }
    bool shouldAbort() const { return errorCount_ >= maxErrors_; }

    // Enable/disable color output
    void setUseColor(bool use) { useColor_ = use; }

private:
    void report(const Diagnostic& diag);
    void printColoredLevel(DiagLevel level);
    std::string generateCaret(int column, int length);

    const SourceManager& srcMgr_;
    int errorCount_;
    int warningCount_;
    int maxErrors_;
    bool useColor_;
};

} // namespace pl0

#endif // PL0_DIAGNOSTICS_H
