#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QRegularExpression>

// PL/0 Syntax Highlighter Implementation
PL0Highlighter::PL0Highlighter(QTextDocument *parent): QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keywords
    keywordFormat.setForeground(QColor("#569CD6"));  // Blue
    keywordFormat.setFontWeight(QFont::Bold);
    const QString keywordPatterns[] = {
        "\\bprogram\\b", "\\bconst\\b", "\\bvar\\b", "\\bprocedure\\b",
        "\\bbegin\\b", "\\bend\\b", "\\bif\\b", "\\bthen\\b", "\\belse\\b",
        "\\bwhile\\b", "\\bdo\\b", "\\bfor\\b", "\\bto\\b", "\\bdownto\\b",
        "\\bcall\\b", "\\bread\\b", "\\bwrite\\b", "\\bodd\\b", "\\bmod\\b",
        "\\bnew\\b", "\\bdelete\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Numbers
    numberFormat.setForeground(QColor("#B5CEA8"));  // Light green
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // Operators
    operatorFormat.setForeground(QColor("#D4D4D4"));  // Light gray
    rule.pattern = QRegularExpression("[+\\-*/<>=:;,\\.\\(\\)\\[\\]]");
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Assignment operator
    rule.pattern = QRegularExpression(":=");
    rule.format = operatorFormat;
    highlightingRules.append(rule);

    // Single-line comments //
    commentFormat.setForeground(QColor("#6A9955"));  // Green
    commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // Pascal-style comments { }
    rule.pattern = QRegularExpression("\\{[^}]*\\}");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void PL0Highlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle multi-line block comments /* */
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(QRegularExpression("/\\*"));

    while (startIndex >= 0) {
        QRegularExpressionMatch match = QRegularExpression("\\*/").match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + match.capturedLength();
        }
        setFormat(startIndex, commentLength, commentFormat);
        startIndex = text.indexOf(QRegularExpression("/\\*"), startIndex + commentLength);
    }
}

// Code Editor Implementation
CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), errorLine_(-1)
{
    lineNumberArea = new LineNumberArea(this);
    highlighter = new PL0Highlighter(document());

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    // Set dark theme
    QPalette p = palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    setPalette(p);

    // Set monospace font
    // Use system default fixed-width font
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    
    // Ensure the size is reasonable if system default is too small/large
    if (font.pointSize() < 20) {
        font.setPointSize(28);
    }
    
    setFont(font);

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor("#2D2D2D");
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#1E1E1E"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNumber = blockNumber + 1;
            QString number = QString::number(lineNumber);
            
            // Draw breakpoint indicator (red circle)
            if (breakpoints_.contains(lineNumber)) {
                painter.setBrush(QColor("#E51400"));  // Red
                painter.setPen(Qt::NoPen);
                int circleSize = fontMetrics().height() - 4;
                int circleY = top + (fontMetrics().height() - circleSize) / 2;
                painter.drawEllipse(3, circleY, circleSize, circleSize);
            }
            
            if (lineNumber == errorLine_) {
                painter.fillRect(0, top, lineNumberArea->width(), fontMetrics().height(), QColor("#5A1D1D"));
                painter.setPen(QColor("#FF6B6B"));
            } else {
                painter.setPen(QColor("#858585"));
            }
            
            painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::setErrorLine(int line)
{
    errorLine_ = line;
    lineNumberArea->update();
}

void CodeEditor::clearErrorLine()
{
    errorLine_ = -1;
    lineNumberArea->update();
}

void CodeEditor::highlightLine(int line, const QColor& color)
{
    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    if (block.isValid()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(color);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = QTextCursor(block);
        selection.cursor.clearSelection();
        
        QList<QTextEdit::ExtraSelection> extraSelections = this->extraSelections();
        extraSelections.append(selection);
        setExtraSelections(extraSelections);
    }
}

void CodeEditor::clearHighlights()
{
    highlightCurrentLine();
}

// Breakpoint methods
void CodeEditor::toggleBreakpoint(int line)
{
    if (breakpoints_.contains(line)) {
        breakpoints_.remove(line);
        Q_EMIT breakpointToggled(line, false);
    } else {
        breakpoints_.insert(line);
        Q_EMIT breakpointToggled(line, true);
    }
    lineNumberArea->update();
}

bool CodeEditor::hasBreakpoint(int line) const
{
    return breakpoints_.contains(line);
}

void CodeEditor::clearBreakpoints()
{
    breakpoints_.clear();
    lineNumberArea->update();
}

int CodeEditor::lineAtPosition(int y) const
{
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while (block.isValid()) {
        if (y >= top && y < bottom) {
            return block.blockNumber() + 1;
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
    return -1;  // Not found
}

// LineNumberArea mouse click handler
void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int lineNumber = codeEditor->lineAtPosition(event->y());
        if (lineNumber > 0) {
            codeEditor->toggleBreakpoint(lineNumber);
        }
    }
    QWidget::mousePressEvent(event);
}
