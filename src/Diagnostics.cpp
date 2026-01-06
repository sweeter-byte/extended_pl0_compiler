#include "Diagnostics.h"
#include "SourceManager.h"
#include "Common.h"
#include <iostream>

namespace pl0 {

DiagnosticsEngine::DiagnosticsEngine(const SourceManager& srcMgr)
    : srcMgr_(srcMgr), errorCount_(0), warningCount_(0), maxErrors_(100), useColor_(isTerminal()) {}

void DiagnosticsEngine::error(const std::string& msg, int line, int col, int len) {
    errorCount_++;
    report(Diagnostic(DiagLevel::ERROR, msg, line, col, len));
}

void DiagnosticsEngine::error(const std::string& msg, const Token& tok) {
    error(msg, tok.line, tok.column, tok.length);
}

void DiagnosticsEngine::warning(const std::string& msg, int line, int col, int len) {
    warningCount_++;
    report(Diagnostic(DiagLevel::WARNING, msg, line, col, len));
}

void DiagnosticsEngine::warning(const std::string& msg, const Token& tok) {
    warning(msg, tok.line, tok.column, tok.length);
}

void DiagnosticsEngine::note(const std::string& msg, int line, int col, int len) {
    report(Diagnostic(DiagLevel::NOTE, msg, line, col, len));
}

void DiagnosticsEngine::report(const Diagnostic& diag) {
    // Format: filename:line:col: level: message
    if (useColor_) {
        std::cerr << Color::Bold << Color::White;
    }
    
    std::cerr << srcMgr_.getFilename() << ":" << diag.line << ":" << diag.column << ": ";
    
    printColoredLevel(diag.level);
    
    if (useColor_) {
        std::cerr << Color::Bold << Color::White;
    }
    std::cerr << diag.message;
    
    if (useColor_) {
        std::cerr << Color::Reset;
    }
    std::cerr << "\n";
    
    // Source line echo
    std::string line = srcMgr_.getLine(diag.line);
    if (!line.empty()) {
        std::cerr << "    " << line << "\n";
        
        // Generate caret indicator ^~~~
        if (useColor_) {
            std::cerr << Color::Green;
        }
        std::cerr << "    " << generateCaret(diag.column, diag.length);
        if (useColor_) {
            std::cerr << Color::Reset;
        }
        std::cerr << "\n";
    }
}

void DiagnosticsEngine::printColoredLevel(DiagLevel level) {
    if (useColor_) {
        switch (level) {
            case DiagLevel::ERROR:
                std::cerr << Color::Bold << Color::Red << "error: " << Color::Reset;
                break;
            case DiagLevel::WARNING:
                std::cerr << Color::Bold << Color::Yellow << "warning: " << Color::Reset;
                break;
            case DiagLevel::NOTE:
                std::cerr << Color::Bold << Color::Cyan << "note: " << Color::Reset;
                break;
        }
    } else {
        switch (level) {
            case DiagLevel::ERROR:
                std::cerr << "error: ";
                break;
            case DiagLevel::WARNING:
                std::cerr << "warning: ";
                break;
            case DiagLevel::NOTE:
                std::cerr << "note: ";
                break;
        }
    }
}

std::string DiagnosticsEngine::generateCaret(int column, int length) {
    std::string result;
    
    // Add leading spaces (column is 1-based)
    for (int i = 1; i < column; i++) {
        result += ' ';
    }
    
    // Add caret ^
    result += '^';
    
    // Add tilde underline ~~~
    for (int i = 1; i < length; i++) {
        result += '~';
    }
    
    return result;
}

} // namespace pl0
