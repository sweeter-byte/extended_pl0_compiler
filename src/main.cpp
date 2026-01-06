#include "Common.h"
#include "Token.h"
#include "Lexer.h"
#include "Parser.h"
#include "SymbolTable.h"
#include "Instruction.h"
#include "Interpreter.h"
#include "SourceManager.h"
#include "Diagnostics.h"
#include "Optimizer.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <sstream>

namespace fs = std::filesystem;

constexpr const char* VERSION = "1.0.0";
constexpr const char* PROGRAM_NAME = "pl0c";

namespace TermColor {
    inline const char* Reset   = "\033[0m";
    inline const char* Bold    = "\033[1m";
    inline const char* Red     = "\033[31m";
    inline const char* Green   = "\033[32m";
    inline const char* Yellow  = "\033[33m";
    inline const char* Blue    = "\033[34m";
    inline const char* Magenta = "\033[35m";
    inline const char* Cyan    = "\033[36m";
    inline const char* White   = "\033[37m";
    inline const char* BoldRed    = "\033[1;31m";
    inline const char* BoldGreen  = "\033[1;32m";
    inline const char* BoldYellow = "\033[1;33m";
    inline const char* BoldCyan   = "\033[1;36m";
    inline const char* BoldBlue   = "\033[1;34m";
}

static bool g_useColor = true;

inline const char* col(const char* color) {
    return g_useColor ? color : "";
}

struct CompilerOptions {
    std::string inputFile;
    bool showTokens   = false;
    bool showAst      = false;
    bool showSymbols  = false;
    bool showCode     = false;
    bool showAll      = false;
    bool noRun        = false;
    bool trace        = false;
    bool noColor      = false;
    bool showHelp     = false;
    bool showVersion  = false;
    bool testMode     = false;
    std::string testDirectory;
    bool optimize     = false;
    bool debug        = false;
};



void printHelp(const char* programName) {

    
    std::cout << col(TermColor::Bold) << "USAGE:" << col(TermColor::Reset) << "\n"
              << "    " << programName << " [OPTIONS] <source_file>\n"
              << "    " << programName << " --test [directory]\n\n";
    
    std::cout << col(TermColor::Bold) << "DESCRIPTION:" << col(TermColor::Reset) << "\n"
              << "    Compiles Extended PL/0 source files to P-Code and executes them.\n"
              << "    Supports arrays, for-loops, heap allocation, and procedures.\n\n";
    
    std::cout << col(TermColor::Bold) << "OPTIONS:" << col(TermColor::Reset) << "\n";
    
    auto printOpt = [](const char* opt, const char* desc) {
        std::cout << "    " << col(TermColor::Green) << std::left << std::setw(20) << opt 
                  << col(TermColor::Reset) << desc << "\n";
    };
    
    printOpt("-h, --help", "Display this help message and exit");
    printOpt("-v, --version", "Display version information and exit");
    printOpt("--tokens", "Print lexer token sequence");
    printOpt("--ast", "Print abstract syntax tree");
    printOpt("--sym", "Print symbol table");
    printOpt("--code", "Print generated P-Code instructions");
    printOpt("--all", "Enable all debug outputs (tokens, ast, sym, code)");
    printOpt("--trace", "Trace P-Code execution step by step");
    printOpt("--no-run", "Compile only, do not execute");
    printOpt("--no-color", "Disable colored output");
    printOpt("--test [dir]", "Run batch tests on directory (default: test/)");
    printOpt("-O, --optimize", "Enable optimizations (Const Folding, Dead Code)");
    printOpt("-d, --debug", "Enable interactive debug mode");
    
    std::cout << "\n" << col(TermColor::Bold) << "FILE RESOLUTION:" << col(TermColor::Reset) << "\n"
              << "    The compiler intelligently searches for source files:\n"
              << "    1. Current directory (with/without .pl0 extension)\n"
              << "    2. test/ and ../test/ directories\n"
              << "    3. Module subdirectories: lexer/, parser/, semantic/,\n"
              << "       codegen/, heap/, integration/ (correct/ and error/)\n\n";
    
    std::cout << col(TermColor::Bold) << "EXAMPLES:" << col(TermColor::Reset) << "\n"
              << "    " << col(TermColor::Cyan) << programName << " hello.pl0" 
              << col(TermColor::Reset) << "              # Compile and run\n"
              << "    " << col(TermColor::Cyan) << programName << " test_heap --code" 
              << col(TermColor::Reset) << "          # Show P-Code for test_heap.pl0\n"
              << "    " << col(TermColor::Cyan) << programName << " --all program.pl0" 
              << col(TermColor::Reset) << "         # Full debug output\n"
              << "    " << col(TermColor::Cyan) << programName << " --test" 
              << col(TermColor::Reset) << "                     # Run all tests\n"
              << "    " << col(TermColor::Cyan) << programName << " --test test/parser" 
              << col(TermColor::Reset) << "         # Test parser module only\n\n";
    
    std::cout << col(TermColor::Bold) << "TEST DIRECTORY STRUCTURE:" << col(TermColor::Reset) << "\n"
              << "    test/\n"
              << "    ├── lexer/\n"
              << "    │   ├── correct/    " << col(TermColor::Green) << "# Files expected to compile" << col(TermColor::Reset) << "\n"
              << "    │   └── error/      " << col(TermColor::Red) << "# Files expected to fail" << col(TermColor::Reset) << "\n"
              << "    ├── parser/\n"
              << "    └── ...\n\n";
    
    std::cout << col(TermColor::Bold) << "EXIT CODES:" << col(TermColor::Reset) << "\n"
              << "    0  Success\n"
              << "    1  Compilation error\n"
              << "    2  Runtime error\n"
              << "    3  File not found\n"
              << "    4  Invalid arguments\n\n";
}

void printVersion() {
    std::cout << col(TermColor::BoldCyan) << "Extended PL/0 Compiler" << col(TermColor::Reset)
              << " version " << col(TermColor::Bold) << VERSION << col(TermColor::Reset) << "\n"
              << "Copyright (c) 2025. Licensed under MIT.\n"
              << "Built with C++17 for Linux/Ubuntu.\n";
}

class FileResolver {
public:
    static std::string resolve(const std::string& filename) {
        // List of search directories
        static const std::vector<std::string> searchDirs = {
            ".",
            "test",
            "../test",
            "tests",
            "../tests"
        };
        
        // Module subdirectories within test folders
        static const std::vector<std::string> modules = {
            "lexer", "parser", "semantic", "codegen", 
            "heap", "integration", "procedure", "array",
            "diagnostics", "interpreter", "unit"
        };
        
        // Subdirectories within each module
        static const std::vector<std::string> subDirs = {
            "correct", "error", ""
        };
        
        std::vector<std::string> candidates;
        
        // Generate candidate paths
        auto addCandidates = [&](const std::string& base) {
            candidates.push_back(base);
            if (!hasExtension(base, ".pl0")) {
                candidates.push_back(base + ".pl0");
            }
        };
        
        // 1. Check original filename as-is
        addCandidates(filename);
        
        // 2. Check in search directories
        for (const auto& dir : searchDirs) {
            addCandidates(dir + "/" + filename);
            
            // 3. Check in module subdirectories
            for (const auto& mod : modules) {
                for (const auto& sub : subDirs) {
                    std::string path = dir + "/" + mod;
                    if (!sub.empty()) {
                        path += "/" + sub;
                    }
                    addCandidates(path + "/" + filename);
                }
            }
        }
        
        // Try each candidate
        for (const auto& candidate : candidates) {
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                return fs::canonical(candidate).string();
            }
        }
        
        // Return original filename if not found (will trigger error later)
        return filename;
    }
    
    static bool hasExtension(const std::string& path, const std::string& ext) {
        if (path.length() < ext.length()) return false;
        return path.compare(path.length() - ext.length(), ext.length(), ext) == 0;
    }
    
    static std::string getBasename(const std::string& path) {
        return fs::path(path).stem().string();
    }
    
    static std::string getFilename(const std::string& path) {
        return fs::path(path).filename().string();
    }
};

void printTokens(const std::vector<pl0::Token>& tokens) {
    std::cout << "\n" << col(TermColor::BoldCyan) << "[Lexer]" << col(TermColor::Reset)
              << " Token Sequence:\n";
    std::cout << std::string(76, '-') << "\n";
    std::cout << col(TermColor::Bold)
              << "| " << std::left << std::setw(6) << "Line"
              << "| " << std::setw(6) << "Col"
              << "| " << std::setw(15) << "Type"
              << "| " << std::setw(40) << "Value"
              << "|\n" << col(TermColor::Reset);
    std::cout << std::string(76, '-') << "\n";
    
    for (const auto& tok : tokens) {
        std::cout << "| " << std::left << std::setw(6) << tok.line
                  << "| " << std::setw(6) << tok.column
                  << "| " << std::setw(15) << pl0::tokenTypeToString(tok.type)
                  << "| " << std::setw(40) << tok.literal
                  << "|\n";
    }
    
    std::cout << std::string(76, '-') << "\n";
    std::cout << "Total tokens: " << col(TermColor::Bold) << tokens.size() 
              << col(TermColor::Reset) << "\n";
}

struct CompilationResult {
    bool success = false;
    int errorCount = 0;
    int warningCount = 0;
    std::string errorMessage;
    bool runtimeSuccess = true;
    std::string runtimeError;
};

CompilationResult compileFile(const std::string& filepath, const CompilerOptions& opts) {
    CompilationResult result;
    
    // Load source file
    pl0::SourceManager srcMgr;
    if (!srcMgr.loadFile(filepath)) {
        result.errorMessage = "Failed to open file: " + filepath;
        return result;
    }
    
    // Initialize components
    pl0::DiagnosticsEngine diag(srcMgr);
    pl0::Lexer lexer(srcMgr.getSource(), diag);
    pl0::SymbolTable symTable;
    pl0::CodeGenerator codeGen;
    
    // Tokenize first (for display purposes) - before creating parser
    std::vector<pl0::Token> tokens = lexer.tokenize();
    
    if (opts.showTokens || opts.showAll) {
        printTokens(tokens);
    }
    
    // Reset lexer for parsing
    lexer.reset();
    
    // Create parser after lexer is reset (parser constructor calls advance())
    pl0::Parser parser(lexer, symTable, codeGen, diag);
    
    // Enable AST dump if requested
    if (opts.showAst || opts.showAll) {
        parser.enableAstDump(true);
    }
    
    // Parse and generate code
    parser.parse();

    // Optimize
    if (opts.optimize) {
        pl0::Optimizer optimizer;
        std::vector<pl0::Instruction> optimCode = optimizer.optimize(codeGen.getCode());
        codeGen.setCode(optimCode);
    }
    
    // Show symbol table
    if (opts.showSymbols || opts.showAll) {
        symTable.dump();
    }
    
    // Show generated code
    if (opts.showCode || opts.showAll) {
        codeGen.dump();
    }
    
    // Get error/warning counts
    result.errorCount = diag.getErrorCount();
    result.warningCount = diag.getWarningCount();
    
    // Print compilation summary
    std::cout << "\n" << std::string(50, '=') << "\n";
    if (result.errorCount == 0) {
        std::cout << col(TermColor::BoldGreen) << "Compilation successful" << col(TermColor::Reset);
    } else {
        std::cout << col(TermColor::BoldRed) << "Compilation failed" << col(TermColor::Reset);
    }
    std::cout << " (errors: " << col(result.errorCount > 0 ? TermColor::Red : TermColor::Green) 
              << result.errorCount << col(TermColor::Reset)
              << ", warnings: " << col(result.warningCount > 0 ? TermColor::Yellow : TermColor::Green)
              << result.warningCount << col(TermColor::Reset) << ")\n";
    
    if (result.errorCount > 0) {
        return result;
    }
    
    result.success = true;
    
    // Execute if requested
    if (!opts.noRun) {
        std::cout << "\n" << col(TermColor::BoldCyan) 
                  << "========== Program Execution ==========" 
                  << col(TermColor::Reset) << "\n";
        
        pl0::Interpreter interpreter(codeGen.getCode());
        interpreter.setSymbolTable(&symTable); // Link SymbolTable for debugging
        
        if (opts.trace) {
            interpreter.enableTrace(true);
        }
        
        if (opts.debug) {
            std::cout << col(TermColor::Yellow) << "Entering Debug Mode...\n" << col(TermColor::Reset);
            std::cout << "Commands: b <line> (break), r (run), s (step), n (next), p <var> (print), q (quit)\n";
            
            interpreter.setDebugMode(true);
            interpreter.start(); // Prepare
            
            // REPL loop
            std::string line;
            bool quit = false;
            
            while (!quit) {
                pl0::DebugState state = interpreter.getDebugState();
                if (state == pl0::DebugState::HALTED || state == pl0::DebugState::ERROR) {
                     std::cout << "Program terminated.\n";
                     break; 
                }
                
                int currentLine = interpreter.getCurrentLine();
                std::cout << col(TermColor::BoldBlue) << "(debug L" << currentLine << ")> " << col(TermColor::Reset);
                
                if (!std::getline(std::cin, line)) break;
                if (line.empty()) continue;
                
                std::stringstream ss(line);
                char cmd;
                ss >> cmd;
                
                if (cmd == 'b') {
                     int ln;
                     if (ss >> ln) {
                         interpreter.setBreakpoint(ln);
                         std::cout << "Breakpoint set at line " << ln << "\n";
                     } else {
                         std::cout << "Usage: b <line_number>\n";
                     }
                } else if (cmd == 'r' || cmd == 'c') {
                    interpreter.resume();
                } else if (cmd == 's') {
                    interpreter.step();
                } else if (cmd == 'n') {
                    interpreter.stepOver();
                } else if (cmd == 'p') {
                    std::string var;
                    if (ss >> var) {
                         int val = interpreter.getValue(var);
                         // Note: getValue returns fallback error values if not found or visible.
                         // Ideally we should have a `bool tryGetValue(name, &val)`
                         std::cout << var << " = " << val << "\n";
                    } else {
                         std::cout << "Usage: p <variable_name>\n";
                    }
                } else if (cmd == 'q') {
                    quit = true;
                } else {
                    std::cout << "Unknown command.\n";
                }
            }
            
        } else {
            // Normal Run
            interpreter.run();
        }
        
        if (interpreter.hasError()) {
            result.runtimeSuccess = false;
            result.runtimeError = interpreter.getError();
        }
        
        std::cout << col(TermColor::BoldCyan) 
                  << "========== Execution Complete ==========" 
                  << col(TermColor::Reset) << "\n";
    }
    
    return result;
}

struct TestResult {
    std::string name;
    std::string path;
    bool passed;
    bool expectError;
    std::string message;
    double duration_ms;
};

class TestRunner {
public:
    explicit TestRunner(const std::string& baseDir) : baseDir_(baseDir) {}
    
    std::vector<TestResult> runAllTests() {
        std::vector<TestResult> results;
        
        if (!fs::exists(baseDir_)) {
            std::cerr << col(TermColor::Red) << "Error: " << col(TermColor::Reset)
                      << "Test directory not found: " << baseDir_ << "\n";
            return results;
        }
        
        std::vector<std::pair<std::string, bool>> testFiles;
        collectTestFiles(baseDir_, testFiles);
        
        if (testFiles.empty()) {
            std::cerr << col(TermColor::Yellow) << "Warning: " << col(TermColor::Reset)
                      << "No test files found in " << baseDir_ << "\n";
            return results;
        }
        
        std::sort(testFiles.begin(), testFiles.end());
        
        for (const auto& [path, expectError] : testFiles) {
            results.push_back(runSingleTest(path, expectError));
        }
        
        return results;
    }
    
    static void printResults(const std::vector<TestResult>& results) {
        if (results.empty()) {
            return;
        }
        
        std::cout << "\n" << col(TermColor::BoldCyan)
                  << "╔═══════════════════════════════════════════════════════════╗\n"
                  << "║                     TEST RESULTS                          ║\n"
                  << "╚═══════════════════════════════════════════════════════════╝"
                  << col(TermColor::Reset) << "\n\n";
        
        int passed = 0, failed = 0;
        double totalTime = 0;
        
        std::string currentDir;
        
        for (const auto& r : results) {
            std::string dir = fs::path(r.path).parent_path().string();
            
            if (dir != currentDir) {
                if (!currentDir.empty()) {
                    std::cout << "\n";
                }
                currentDir = dir;
                std::cout << col(TermColor::Bold) << "  " << dir << "/" 
                          << col(TermColor::Reset) << "\n";
            }
            
            if (r.passed) {
                passed++;
                std::cout << "    " << col(TermColor::BoldGreen) << "[PASS]" << col(TermColor::Reset);
            } else {
                failed++;
                std::cout << "    " << col(TermColor::BoldRed) << "[FAIL]" << col(TermColor::Reset);
            }
            
            std::cout << " " << std::left << std::setw(35) << r.name;
            std::cout << col(TermColor::Cyan) << std::right << std::setw(8) 
                      << std::fixed << std::setprecision(2) << r.duration_ms << " ms"
                      << col(TermColor::Reset);
            
            if (!r.message.empty() && !r.passed) {
                std::cout << "  " << col(TermColor::Yellow) << r.message << col(TermColor::Reset);
            }
            
            std::cout << "\n";
            totalTime += r.duration_ms;
        }
        
        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << col(TermColor::Bold) << "SUMMARY:" << col(TermColor::Reset) << "\n";
        std::cout << "  Total:  " << col(TermColor::Bold) << (passed + failed) << col(TermColor::Reset) << " tests\n";
        std::cout << "  Passed: " << col(TermColor::BoldGreen) << passed << col(TermColor::Reset) << "\n";
        std::cout << "  Failed: " << col(failed > 0 ? TermColor::BoldRed : TermColor::BoldGreen) 
                  << failed << col(TermColor::Reset) << "\n";
        std::cout << "  Time:   " << col(TermColor::Cyan) << std::fixed << std::setprecision(2) 
                  << totalTime << " ms" << col(TermColor::Reset) << "\n";
        std::cout << std::string(60, '-') << "\n";
        
        if (failed == 0) {
            std::cout << col(TermColor::BoldGreen) << "\n✓ All tests passed!" 
                      << col(TermColor::Reset) << "\n";
        } else {
            std::cout << col(TermColor::BoldRed) << "\n✗ " << failed << " test(s) failed!" 
                      << col(TermColor::Reset) << "\n";
        }
    }
    
private:
    std::string baseDir_;
    
    void collectTestFiles(const std::string& dir, 
                          std::vector<std::pair<std::string, bool>>& files) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                
                std::string path = entry.path().string();
                std::string ext = entry.path().extension().string();
                
                if (ext != ".pl0") continue;
                
                bool expectError = isErrorTest(path);
                files.emplace_back(path, expectError);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << col(TermColor::Red) << "Error scanning directory: " 
                      << col(TermColor::Reset) << e.what() << "\n";
        }
    }
    
    bool isErrorTest(const std::string& path) const {
        return path.find("/error/") != std::string::npos ||
               path.find("/errors/") != std::string::npos ||
               path.find("\\error\\") != std::string::npos ||
               path.find("\\errors\\") != std::string::npos;
    }
    
    TestResult runSingleTest(const std::string& path, bool expectError) {
        TestResult result;
        result.path = path;
        result.name = fs::path(path).filename().string();
        result.expectError = expectError;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::streambuf* coutBuf = std::cout.rdbuf();
            std::streambuf* cerrBuf = std::cerr.rdbuf();
            std::ostringstream nullStream;
            std::cout.rdbuf(nullStream.rdbuf());
            std::cerr.rdbuf(nullStream.rdbuf());
            
            CompilerOptions opts;
            opts.noColor = true;
            
            if (path.find("interpreter") != std::string::npos || 
                path.find("integration") != std::string::npos) {
                opts.noRun = false;
            } else {
                opts.noRun = true;
            }
            
            CompilationResult compResult = compileFile(path, opts);
            
            std::cout.rdbuf(coutBuf);
            std::cerr.rdbuf(cerrBuf);
            
            bool hasErrors = compResult.errorCount > 0;
            bool runtimeFailed = !compResult.runtimeSuccess;
            
            if (expectError) {
                result.passed = hasErrors || runtimeFailed;
                if (!result.passed) {
                    result.message = "Expected error but compiled and ran successfully";
                }
            } else {
                result.passed = !hasErrors && !runtimeFailed;
                if (hasErrors) {
                    result.message = "Unexpected compilation error";
                } else if (runtimeFailed) {
                    result.message = "Unexpected runtime error: " + compResult.runtimeError;
                }
            }
            
        } catch (const std::exception& e) {
            result.passed = expectError;
            result.message = std::string("Exception: ") + e.what();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        return result;
    }
};

CompilerOptions parseArguments(int argc, char* argv[]) {
    CompilerOptions opts;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            opts.showHelp = true;
        } else if (arg == "-v" || arg == "--version") {
            opts.showVersion = true;
        } else if (arg == "--tokens") {
            opts.showTokens = true;
        } else if (arg == "--ast") {
            opts.showAst = true;
        } else if (arg == "--sym") {
            opts.showSymbols = true;
        } else if (arg == "--code") {
            opts.showCode = true;
        } else if (arg == "--all") {
            opts.showAll = true;
        } else if (arg == "--trace") {
            opts.trace = true;
        } else if (arg == "--no-run") {
            opts.noRun = true;
        } else if (arg == "--no-color") {
            opts.noColor = true;
            g_useColor = false;
        } else if (arg == "--test") {
            opts.testMode = true;
            // Check if next argument is a directory
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                opts.testDirectory = argv[++i];
            } else {
                opts.testDirectory = "test";  // Default
            }
        } else if (arg == "--optimize" || arg == "-O") {
            opts.optimize = true;
        } else if (arg == "--debug" || arg == "-d") {
            opts.debug = true;
        } else if (arg[0] == '-') {
            std::cerr << col(TermColor::Red) << "Error: " << col(TermColor::Reset)
                      << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            std::exit(4);
        } else {
            if (opts.inputFile.empty()) {
                opts.inputFile = arg;
            } else {
                std::cerr << col(TermColor::Red) << "Error: " << col(TermColor::Reset)
                          << "Multiple input files specified.\n";
                std::exit(4);
            }
        }
    }
    
    return opts;
}

int main(int argc, char* argv[]) {
    // Check for terminal color support
    if (!pl0::isTerminal()) {
        g_useColor = false;
    }
    
    // Parse command-line arguments
    CompilerOptions opts = parseArguments(argc, argv);
    
    // Handle --no-color early
    if (opts.noColor) {
        g_useColor = false;
    }
    
    // Handle help/version
    if (opts.showHelp) {
        printHelp(argv[0]);
        return 0;
    }
    
    if (opts.showVersion) {
        printVersion();
        return 0;
    }
    
    // Handle test mode
    if (opts.testMode) {

        std::cout << col(TermColor::Bold) << "Running tests in: " << col(TermColor::Reset)
                  << opts.testDirectory << "\n";
        
        TestRunner runner(opts.testDirectory);
        auto results = runner.runAllTests();
        TestRunner::printResults(results);
        
        // Return non-zero if any test failed
        int failed = std::count_if(results.begin(), results.end(),
                                   [](const TestResult& r) { return !r.passed; });
        return failed > 0 ? 1 : 0;
    }
    
    // Check for input file
    if (opts.inputFile.empty()) {
        printHelp(argv[0]);
        return 0;
    }
    
    // Resolve file path
    std::string resolvedPath = FileResolver::resolve(opts.inputFile);
    
    // Check if file exists
    if (!fs::exists(resolvedPath)) {
        std::cerr << col(TermColor::BoldRed) << "Error: " << col(TermColor::Reset)
                  << "File not found: " << opts.inputFile << "\n";
        
        // Suggest similar files
        std::string dir = fs::path(opts.inputFile).parent_path().string();
        if (dir.empty()) dir = ".";
        
        if (fs::exists(dir)) {
            std::vector<std::string> suggestions;
            std::string base = FileResolver::getBasename(opts.inputFile);
            
            try {
                for (const auto& entry : fs::directory_iterator(dir)) {
                    if (entry.path().extension() == ".pl0") {
                        std::string name = entry.path().stem().string();
                        // Simple similarity check
                        if (name.find(base) != std::string::npos ||
                            base.find(name) != std::string::npos) {
                            suggestions.push_back(entry.path().filename().string());
                        }
                    }
                }
            } catch (...) {}
            
            if (!suggestions.empty()) {
                std::cerr << "\nDid you mean:\n";
                for (const auto& s : suggestions) {
                    std::cerr << "  " << col(TermColor::Cyan) << s << col(TermColor::Reset) << "\n";
                }
            }
        }
        
        return 3;
    }
    
    // Print header
    std::cout << col(TermColor::BoldCyan) << "Extended PL/0 Compiler" << col(TermColor::Reset) << "\n";
    std::cout << "Input file: " << col(TermColor::Bold) << resolvedPath << col(TermColor::Reset) << "\n";
    std::cout << std::string(50, '=') << "\n";
    
    // Compile
    CompilationResult result = compileFile(resolvedPath, opts);
    
    if (!result.success) {
        if (!result.errorMessage.empty()) {
            std::cerr << col(TermColor::Red) << "Error: " << col(TermColor::Reset)
                      << result.errorMessage << "\n";
        }
        return result.errorCount > 0 ? 1 : 2;
    }
    
    return 0;
}
