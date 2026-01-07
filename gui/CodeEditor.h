#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QSet>
#include <QMouseEvent>

class LineNumberArea;

// PL/0 Syntax Highlighter
class PL0Highlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit PL0Highlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
};

// Code Editor with Line Numbers
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    
    void setErrorLine(int line);
    void clearErrorLine();
    void highlightLine(int line, const QColor& color);
    void clearHighlights();
    
    // Breakpoint management
    void toggleBreakpoint(int line);
    bool hasBreakpoint(int line) const;
    QSet<int> getBreakpoints() const { return breakpoints_; }
    void clearBreakpoints();
    
    // Helper for LineNumberArea
    int lineAtPosition(int y) const;

Q_SIGNALS:
    void breakpointToggled(int line, bool enabled);

protected:
    void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget *lineNumberArea;
    PL0Highlighter *highlighter;
    int errorLine_;
    QSet<int> breakpoints_;  // Lines with breakpoints
};

// Line Number Area Widget
class LineNumberArea : public QWidget {
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }
    
    void mousePressEvent(QMouseEvent *event) override;

private:
    CodeEditor *codeEditor;
};

#endif // CODEEDITOR_H
