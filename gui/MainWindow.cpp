// Define before Qt headers to avoid keyword conflicts with 'emit'
#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#endif

#include "MainWindow.h"
#include "CodeEditor.h"
#include "ConsoleWidget.h"
#include "../include/Common.h"
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/SymbolTable.h"
#include "../include/Instruction.h"
#include "../include/Interpreter.h"
#include "../include/SourceManager.h"
#include "../include/Diagnostics.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFontDatabase>
#include <QHeaderView>
#include <QRegExp>
#include <QTextStream>
#include <QFileInfo>
#include <iostream>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isModified_(false)
    , isDebugging_(false)
    , currentDebugLine_(-1)
    , baseFontSize_(13)  // Larger default size
    , currentFontSize_(13)
{
    setupUI();
    createActions();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    connectSignals();
    
    setWindowTitle("PL/0 Compiler - [Untitled]");
    resize(1400, 900);
}

MainWindow::~MainWindow() {
    // Qt's parent-child system will handle cleanup
}

void MainWindow::setupUI() {
    // Central widget and main layout
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    
    // Main horizontal splitter
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    
    // Left side: Code editor
    codeEditor_ = new CodeEditor(this);
    
    // Right side: Tab widget for visualizations
    rightPanel_ = new QTabWidget(this);
    rightPanel_->setMinimumWidth(400);
    
    // Token view tab
    tokenTable_ = new QTableWidget(this);
    tokenTable_->setColumnCount(4);
    tokenTable_->setHorizontalHeaderLabels({"Type", "Value", "Line", "Column"});
    tokenTable_->horizontalHeader()->setStretchLastSection(true);
    tokenTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tokenTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    rightPanel_->addTab(tokenTable_, "Tokens");
    
    // AST view tab
    astTree_ = new QTreeWidget(this);
    astTree_->setHeaderLabel("Abstract Syntax Tree");
    astTree_->setExpandsOnDoubleClick(true);
    rightPanel_->addTab(astTree_, "AST");
    
    // Symbol table view tab
    symbolTree_ = new QTreeWidget(this);
    symbolTree_->setColumnCount(5);
    symbolTree_->setHeaderLabels({"Name", "Kind", "Level", "Address", "Value/Size"});
    symbolTree_->header()->setStretchLastSection(false);
    symbolTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    rightPanel_->addTab(symbolTree_, "Symbols");
    
    // P-Code view tab
    pcodeTable_ = new QTableWidget(this);
    pcodeTable_->setColumnCount(4);
    pcodeTable_->setHorizontalHeaderLabels({"Address", "Operation", "Level", "Operand"});
    pcodeTable_->horizontalHeader()->setStretchLastSection(true);
    pcodeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pcodeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    rightPanel_->addTab(pcodeTable_, "P-Code");
    
    mainSplitter_->addWidget(codeEditor_);
    mainSplitter_->addWidget(rightPanel_);
    mainSplitter_->setStretchFactor(0, 3);  // Code editor takes 60%
    mainSplitter_->setStretchFactor(1, 2);  // Right panel takes 40%
    
    // Bottom: Console widget
    console_ = new ConsoleWidget(this);
    console_->setMinimumHeight(150);
    console_->setMaximumHeight(300);
    
    // Bottom splitter for editor and console
    bottomSplitter_ = new QSplitter(Qt::Vertical, this);
    bottomSplitter_->addWidget(mainSplitter_);
    bottomSplitter_->addWidget(console_);
    bottomSplitter_->setStretchFactor(0, 4);
    bottomSplitter_->setStretchFactor(1, 1);
    
    mainLayout->addWidget(bottomSplitter_);
}

void MainWindow::createActions() {
    // File actions
    newAction_ = new QAction(tr("&New"), this);
    newAction_->setShortcut(QKeySequence::New);
    
    openAction_ = new QAction(tr("&Open..."), this);
    openAction_->setShortcut(QKeySequence::Open);
    
    saveAction_ = new QAction(tr("&Save"), this);
    saveAction_->setShortcut(QKeySequence::Save);
    
    saveAsAction_ = new QAction(tr("Save &As..."), this);
    saveAsAction_->setShortcut(QKeySequence::SaveAs);
    
    // Compile and run actions
    compileAction_ = new QAction(tr("&Compile"), this);
    compileAction_->setShortcut(QKeySequence(tr("F5")));
    
    runAction_ = new QAction(tr("&Run"), this);
    runAction_->setShortcut(QKeySequence(tr("F6")));
    
    // Debug actions
    debugAction_ = new QAction(tr("Start &Debug"), this);
    debugAction_->setShortcut(QKeySequence(tr("F7")));
    
    stepAction_ = new QAction(tr("&Step"), this);
    stepAction_->setShortcut(QKeySequence(tr("F8")));
    stepAction_->setEnabled(false);
    
    continueAction_ = new QAction(tr("&Continue"), this);
    continueAction_->setShortcut(QKeySequence(tr("F9")));
    continueAction_->setEnabled(false);
    
    stopAction_ = new QAction(tr("S&top"), this);
    stopAction_->setShortcut(QKeySequence(tr("Shift+F7")));
    stopAction_->setEnabled(false);
    
    // View actions
    zoomInAction_ = new QAction(tr("Zoom &In"), this);
    zoomInAction_->setShortcut(QKeySequence::ZoomIn);
    
    zoomOutAction_ = new QAction(tr("Zoom &Out"), this);
    zoomOutAction_->setShortcut(QKeySequence::ZoomOut);
    
    resetZoomAction_ = new QAction(tr("&Reset Zoom"), this);
    resetZoomAction_->setShortcut(QKeySequence(tr("Ctrl+0")));
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu(tr("&File"));
    fileMenu->addAction(newAction_);
    fileMenu->addAction(openAction_);
    fileMenu->addSeparator();
    fileMenu->addAction(saveAction_);
    fileMenu->addAction(saveAsAction_);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close, QKeySequence::Quit);
    
    // View menu
    QMenu* viewMenu = menuBar->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAction_);
    viewMenu->addAction(zoomOutAction_);
    viewMenu->addAction(resetZoomAction_);
    
    // Build menu
    QMenu* buildMenu = menuBar->addMenu(tr("&Build"));
    buildMenu->addAction(compileAction_);
    buildMenu->addAction(runAction_);
    
    // Debug menu
    QMenu* debugMenu = menuBar->addMenu(tr("&Debug"));
    debugMenu->addAction(debugAction_);
    debugMenu->addAction(stepAction_);
    debugMenu->addAction(continueAction_);
    debugMenu->addAction(stopAction_);
}

void MainWindow::setupToolBar() {
    QToolBar* toolBar = addToolBar(tr("Main Toolbar"));
    toolBar->setMovable(false);
    
    toolBar->addAction(newAction_);
    toolBar->addAction(openAction_);
    toolBar->addAction(saveAction_);
    toolBar->addSeparator();
    toolBar->addAction(compileAction_);
    toolBar->addAction(runAction_);
    toolBar->addSeparator();
    toolBar->addAction(debugAction_);
}

void MainWindow::setupStatusBar() {
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::connectSignals() {
    // File operations
    connect(newAction_, &QAction::triggered, this, &MainWindow::newFile);
    connect(openAction_, &QAction::triggered, this, &MainWindow::openFile);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::saveFile);
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::saveFileAs);
    
    // Compile and run
    connect(compileAction_, &QAction::triggered, this, &MainWindow::compile);
    connect(runAction_, &QAction::triggered, this, &MainWindow::run);
    
    // Debug
    connect(debugAction_, &QAction::triggered, this, &MainWindow::startDebug);
    connect(stepAction_, &QAction::triggered, this, &MainWindow::stepDebug);
    connect(continueAction_, &QAction::triggered, this, &MainWindow::continueDebug);
    connect(stopAction_, &QAction::triggered, this, &MainWindow::stopDebug);
    
    // View
    connect(zoomInAction_, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(zoomOutAction_, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(resetZoomAction_, &QAction::triggered, this, &MainWindow::resetZoom);
}

// File operations
void MainWindow::newFile() {
    codeEditor_->clear();
    currentFilePath_.clear();
    isModified_ = false;
    setWindowTitle("PL/0 Compiler - [Untitled]");
    clearVisualizations();
    console_->clear();
    statusBar()->showMessage(tr("New file created"));
}

void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open PL/0 File"),
        "",
        tr("PL/0 Files (*.pl0);;All Files (*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file: ") + file.errorString());
        return;
    }
    
    QTextStream in(&file);
    in.setCodec("UTF-8");  // Ensure UTF-8 encoding support
    QString content = in.readAll();
    file.close();
    
    codeEditor_->setPlainText(content);
    currentFilePath_ = fileName;
    isModified_ = false;
    setWindowTitle("PL/0 Compiler - " + QFileInfo(fileName).fileName());
    clearVisualizations();
    console_->clear();
    statusBar()->showMessage(tr("File opened: ") + fileName);
}

void MainWindow::saveFile() {
    if (currentFilePath_.isEmpty()) {
        saveFileAs();
        return;
    }
    
    QFile file(currentFilePath_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot save file: ") + file.errorString());
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");  // Ensure UTF-8 encoding support
    out << codeEditor_->toPlainText();
    file.close();
    
    isModified_ = false;
    statusBar()->showMessage(tr("File saved: ") + currentFilePath_);
}

void MainWindow::saveFileAs() {
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save PL/0 File"),
        "",
        tr("PL/0 Files (*.pl0);;All Files (*)")
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
}

// ============================================================================
// Compilation and Execution
// ============================================================================

void MainWindow::compile() {
    console_->clear();
    clearVisualizations();
    codeEditor_->clearErrorLine();
    
    // Get source code (UTF-8 encoded)
    QString sourceCode = codeEditor_->toPlainText();
    std::string sourceStr = sourceCode.toUtf8().constData();
    
    console_->appendInfo("=== Compiling ===");
    
    // Create compiler components
    pl0::SourceManager srcMgr;
    srcMgr.loadString(sourceStr, currentFilePath_.isEmpty() ? "<untitled>" : currentFilePath_.toStdString());
    pl0::DiagnosticsEngine diag(srcMgr);
    diag.setUseColor(false);  // No color in GUI
    pl0::SymbolTable symTable;
    pl0::CodeGenerator codeGen;
    pl0::Lexer lexer(sourceStr, diag);
    pl0::Parser parser(lexer, symTable, codeGen, diag);
    
    // Enable AST dump and capture output (and stderr for diagnostics)
    parser.enableAstDump(true);
    std::ostringstream astCapture;
    std::ostringstream errCapture;
    std::streambuf* oldCout = std::cout.rdbuf(astCapture.rdbuf());
    std::streambuf* oldCerr = std::cerr.rdbuf(errCapture.rdbuf());
    
    // Parse
    bool success = parser.parse();
    
    // Restore streams
    std::cout.rdbuf(oldCout);
    std::cerr.rdbuf(oldCerr);

    astOutput_ = QString::fromUtf8(astCapture.str().c_str());
    QString errorOutput = QString::fromUtf8(errCapture.str().c_str());
    
    // Collect tokens for visualization
    tokens_.clear();
    pl0::Lexer tokenCollector(sourceStr, diag);
    pl0::Token tok;
    while ((tok = tokenCollector.nextToken()).type != pl0::TokenType::END_OF_FILE) {
        if (tok.type != pl0::TokenType::UNKNOWN) {
            tokens_.push_back(std::make_tuple(
                QString::fromStdString(pl0::tokenTypeToString(tok.type)),
                QString::fromUtf8(tok.literal.c_str()),
                tok.line,
                tok.column));
        }
    }
    
    // Collect P-Code
    pcode_.clear();
    const auto& instructions = codeGen.getCode();
    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& instr = instructions[i];
        pcode_.push_back({static_cast<int>(i),
                           QString::fromStdString(pl0::opCodeToString(instr.op)),
                           instr.L,
                           instr.A});
    }
    
    // Capture symbol table dump
    std::ostringstream symCapture;
    std::streambuf* oldCoutSym = std::cout.rdbuf(symCapture.rdbuf());
    symTable.dump();
    std::cout.rdbuf(oldCoutSym);
    symbolOutput_ = QString::fromUtf8(symCapture.str().c_str());
    
    // Update visualizations
    updateTokenView();
    updateASTView();
    updateSymbolView();
    updatePCodeView();
    
    // Show diagnostics
    if (!errorOutput.isEmpty()) {
       console_->appendError(errorOutput);
    }

    if (diag.hasErrors()) {
        console_->appendError("Compilation failed with errors.");
        statusBar()->showMessage(tr("Compilation failed"), 3000);
        
        // Highlight first error line (simplified - would need diag API enhancement)
        codeEditor_->setErrorLine(1);
    } else {
        console_->appendOutput("Compilation successful!");
        statusBar()->showMessage(tr("Compilation successful"),3000);
    }
}

void MainWindow::run() {
    console_->appendInfo("\n=== Running Program ===");
    
    // First compile if needed
    if (pcode_.empty()) {
        compile();
        if (pcode_.empty()) {
            console_->appendError("Cannot run: compilation required");
            return;
        }
    }
    
    // Get source code again to run interpreter
    QString sourceCode = codeEditor_->toPlainText();
    std::string sourceStr = sourceCode.toUtf8().constData();
    
    pl0::SourceManager srcMgr;
    srcMgr.loadString(sourceStr, currentFilePath_.isEmpty() ? "<untitled>" : currentFilePath_.toStdString());
    pl0::DiagnosticsEngine diag(srcMgr);
    diag.setUseColor(false);
    pl0::SymbolTable symTable;
    pl0::CodeGenerator codeGen;
    pl0::Lexer lexer(sourceStr, diag);
    pl0::Parser parser(lexer, symTable, codeGen, diag);
    
    if (!parser.parse()) {
        console_->appendError("Failed to recompile before running");
        return;
    }
    
    // Run interpreter
    pl0::Interpreter interpreter(codeGen.getCode());
    interpreter.enableTrace(false);
    
    // Capture interpreter output
    std::ostringstream outCapture;
    std::streambuf* oldCout = std::cout.rdbuf(outCapture.rdbuf());
    
    try {
        interpreter.run();
        std::cout.rdbuf(oldCout);
        
        QString output = QString::fromUtf8(outCapture.str().c_str());
        if (!output.isEmpty()) {
            console_->appendOutput(output);
        }
        console_->appendInfo("Program finished.");
        statusBar()->showMessage(tr("Execution completed"), 3000);
    } catch (...) {
        std::cout.rdbuf(oldCout);
        console_->appendError("Runtime error occurred");
        statusBar()->showMessage(tr("Execution failed"), 3000);
    }
}

// ============================================================================
// Visualization Updates
// ============================================================================

void MainWindow::updateTokenView() {
    tokenTable_->setRowCount(0);
    
    for (const auto& token : tokens_) {
        int row = tokenTable_->rowCount();
        tokenTable_->insertRow(row);
        
        tokenTable_->setItem(row, 0, new QTableWidgetItem(std::get<0>(token)));  // Type
        tokenTable_->setItem(row, 1, new QTableWidgetItem(std::get<1>(token)));  // Value
        tokenTable_->setItem(row, 2, new QTableWidgetItem(QString::number(std::get<2>(token))));  // Line
        tokenTable_->setItem(row, 3, new QTableWidgetItem(QString::number(std::get<3>(token))));  // Column
    }
    
    statusBar()->showMessage(tr("Token view updated: %1 tokens").arg(tokens_.size()));
}

void MainWindow::updateASTView() {
    astTree_->clear();
    
    if (astOutput_.isEmpty()) {
        QTreeWidgetItem* emptyItem = new QTreeWidgetItem(astTree_);
        emptyItem->setText(0, "(No AST available)");
        return;
    }
    
    // Parse the AST dump output and build tree
    // The AST dump format is indented with "+ NodeName"
    QStringList lines = astOutput_.split('\n', Qt::SkipEmptyParts);
    
    QTreeWidgetItem* root = nullptr;
    QList<QTreeWidgetItem*> stack;
    int lastIndent = -2;
    
    for (const QString& line : lines) {
        // Remove ANSI color codes (simplified)
        QString cleaned = line;
        cleaned.remove(QRegExp("\x1B\\[[0-9;]*m"));
        
        // Count indentation
        int indent = 0;
        while (indent < cleaned.length() && cleaned[indent] == ' ') {
            indent++;
        }
        
        // Extract node name (after "+ ")
        QString nodeName = cleaned.trimmed();
        if (nodeName.startsWith("+ ")) {
            nodeName = nodeName.mid(2);
        }
        
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, nodeName);
        
        // Determine parent based on indentation
        if (indent == 0) {
            astTree_->addTopLevelItem(item);
            root = item;
            stack.clear();
            stack.append(item);
        } else if (indent > lastIndent) {
            // Child of prev node
            if (!stack.isEmpty()) {
                stack.last()->addChild(item);
                stack.append(item);
            }
        } else if (indent == lastIndent) {
            // Sibling
            if (stack.size() > 1) {
                stack.pop_back();
                stack.last()->addChild(item);
                stack.append(item);
            }
        } else {
            // Going back up
            int levels = (lastIndent - indent) / 2 + 1;
            for (int i = 0; i < levels && stack.size() > 1; ++i) {
                stack.pop_back();
            }
            if (!stack.isEmpty()) {
                stack.last()->addChild(item);
                stack.append(item);
            }
        }
        
        lastIndent = indent;
    }
    
    if (root) {
        astTree_->expandAll();
    }
}

void MainWindow::updateSymbolView() {
    symbolTree_->clear();
    
    if (symbolOutput_.isEmpty()) {
        QTreeWidgetItem* emptyItem = new QTreeWidgetItem(symbolTree_);
        emptyItem->setText(0, "(No symbols)");
        return;
    }
    
    // Parse the symbol table dump output
    // Format: | Index| Name           | Kind    | Level | Addr/Val    | Size/Params |
    QStringList lines = symbolOutput_.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        // Remove ANSI color codes
        QString cleaned = line;
        cleaned.remove(QRegExp("\x1B\\[[0-9;]*m"));
        cleaned = cleaned.trimmed();
        
        // Skip non-data lines (headers, delimiters, etc)
        if (!cleaned.startsWith("|")) continue;
        if (cleaned.contains("---")) continue;
        if (cleaned.contains("Index") || cleaned.contains("Name") || cleaned.contains("Kind")) continue;
        if (cleaned.contains("Total symbols:")) continue;
        
        // Parse table row: | idx | name | kind | level | addr | val |
        QStringList parts = cleaned.split('|', Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            QTreeWidgetItem* item = new QTreeWidgetItem(symbolTree_);
            // parts[0]=Index, parts[1]=Name, parts[2]=Kind, parts[3]=Level, parts[4]=Addr, parts[5]=Size
            item->setText(0, parts[1].trimmed());  // Name
            item->setText(1, parts[2].trimmed());  // Kind
            item->setText(2, parts[3].trimmed());  // Level
            item->setText(3, parts[4].trimmed());  // Address
            item->setText(4, parts.size() > 5 ? parts[5].trimmed() : "-");  // Size/Params
        }
    }
    
    if (symbolTree_->topLevelItemCount() == 0) {
        QTreeWidgetItem* emptyItem = new QTreeWidgetItem(symbolTree_);
        emptyItem->setText(0, "(No symbols defined)");
    }
}

void MainWindow::updatePCodeView() {
    pcodeTable_->setRowCount(0);
    
    for (const auto& instr : pcode_) {
        int row = pcodeTable_->rowCount();
        pcodeTable_->insertRow(row);
        
        pcodeTable_->setItem(row, 0, new QTableWidgetItem(QString::number(std::get<0>(instr))));  // Address
        pcodeTable_->setItem(row, 1, new QTableWidgetItem(std::get<1>(instr)));                    // Operation
        pcodeTable_->setItem(row, 2, new QTableWidgetItem(QString::number(std::get<2>(instr))));  // Level
        pcodeTable_->setItem(row, 3, new QTableWidgetItem(QString::number(std::get<3>(instr))));  // Operand
    }
    
    statusBar()->showMessage(tr("P-Code view updated: %1 instructions").arg(pcode_.size()));
}

void MainWindow::clearVisualizations() {
    tokenTable_->setRowCount(0);
    astTree_->clear();
    symbolTree_->clear();
    pcodeTable_->setRowCount(0);
    tokens_.clear();
    pcode_.clear();
    astOutput_.clear();
}

// ============================================================================
// Debug Functions (Stubs)
// ============================================================================

void MainWindow::startDebug() {
    console_->appendInfo("Debug mode not yet implemented");
}

void MainWindow::stepDebug() {
    // To be implemented
}

void MainWindow::continueDebug() {
    // To be implemented
}

void MainWindow::stopDebug() {
    // To be implemented
}

void MainWindow::updateDebugState() {
    // To be implemented
}

void MainWindow::highlightCurrentPCodeLine(int line) {
    // To be implemented
    Q_UNUSED(line);
}

void MainWindow::onCompileFinished() {
    // Slot for future use
}

void MainWindow::onTabChanged(int index) {
    // Slot for future use
    Q_UNUSED(index);
}

// Console helpers
void MainWindow::appendConsoleOutput(const QString& text) {
    console_->appendOutput(text);
}

void MainWindow::appendConsoleError(const QString& text) {
    console_->appendError(text);
}

// ============================================================================
// Font Zoom Functions
// ============================================================================

void MainWindow::zoomIn() {
    currentFontSize_ += 2;
    if (currentFontSize_ > 36) {
        currentFontSize_ = 36;  // Max size
    }
    
    QFont font = codeEditor_->font();
    font.setPointSize(currentFontSize_);
    codeEditor_->setFont(font);
    
    statusBar()->showMessage(tr("Font size: %1").arg(currentFontSize_), 2000);
}

void MainWindow::zoomOut() {
    currentFontSize_ -= 2;
    if (currentFontSize_ < 8) {
        currentFontSize_ = 8;  // Min size
    }
    
    QFont font = codeEditor_->font();
    font.setPointSize(currentFontSize_);
    codeEditor_->setFont(font);
    
    statusBar()->showMessage(tr("Font size: %1").arg(currentFontSize_), 2000);
}

void MainWindow::resetZoom() {
    currentFontSize_ = baseFontSize_;
    
    QFont font = codeEditor_->font();
    font.setPointSize(currentFontSize_);
    codeEditor_->setFont(font);
    
    statusBar()->showMessage(tr("Font size reset to %1").arg(currentFontSize_), 2000);
}

