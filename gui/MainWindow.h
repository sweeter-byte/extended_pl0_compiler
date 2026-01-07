#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QTableWidget>
#include <QTreeWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <memory>
#include <vector>
#include "../include/Instruction.h"
#include "../include/SymbolTable.h"

namespace pl0 {
    class Lexer;
    class Parser;
    class Interpreter;
    class SourceManager;
    class DiagnosticsEngine;
}

class CodeEditor;
class ConsoleWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private Q_SLOTS:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    
    void compile();
    void run();
    void startDebug();
    void stepDebug();
    void continueDebug();
    void stopDebug();
    
    void onCompileFinished();
    void onTabChanged(int index);
    
    void zoomIn();
    void zoomOut();
    void resetZoom();
    
    void onConsoleInput(const QString& input);  // Handle console input during debug

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void createActions();
    void connectSignals();
    
    void updateTokenView();
    void updateASTView();  // New: AST visualization
    void updateSymbolView();
    void updatePCodeView();
    void updateDebugState();
    void updateVariableWatch();     // Debug: show variables with runtime values
    void updateStackVisualization(); // Debug: draw stack diagram
    void highlightCurrentPCodeLine(int line);
    void clearVisualizations();
    
    void appendConsoleOutput(const QString& text);
    void appendConsoleError(const QString& text);
    
    // UI Components
    CodeEditor* codeEditor_;
    QTabWidget* rightPanel_;
    QTableWidget* tokenTable_;
    QTreeWidget* astTree_;       // New: AST tree view
    QTreeWidget* symbolTree_;
    QTableWidget* pcodeTable_;
    ConsoleWidget* console_;
    
    // Debug Panel - Variable Watch and Stack Visualization
    QWidget* debugPanel_;
    QLabel* pcLabel_;
    QLabel* bpLabel_;
    QLabel* spLabel_;
    QTableWidget* stackTable_;
    QTreeWidget* variableWatch_;   // Variable watch with runtime values
    QTextEdit* stackDiagram_;      // ASCII art stack visualization
    
    // Splitters
    QSplitter* mainSplitter_;
    QSplitter* bottomSplitter_;
    
    // Actions
    QAction* newAction_;
    QAction* openAction_;
    QAction* saveAction_;
    QAction* saveAsAction_;
    QAction* compileAction_;
    QAction* runAction_;
    QAction* debugAction_;
    QAction* stepAction_;
    QAction* continueAction_;
    QAction* stopAction_;
    QAction* zoomInAction_;
    QAction* zoomOutAction_;
    QAction* resetZoomAction_;
    
    // Compiler state
    QString currentFilePath_;
    bool isModified_;
    bool isDebugging_;
    int currentDebugLine_;
    std::unique_ptr<pl0::Interpreter> interpreter_;
    
    int baseFontSize_;
    int currentFontSize_;
    
    std::vector<pl0::Instruction> rawInstructions_;
    pl0::SymbolTable symTable_;
    std::vector<std::tuple<QString, QString, int, int>> tokens_;  // type, value, line, column
    std::vector<std::tuple<int, QString, int, int>> pcode_;  // addr, op, l, a
    QString astOutput_;  // AST dump output
    QString symbolOutput_;  // Symbol table dump output
};

#endif // MAINWINDOW_H
