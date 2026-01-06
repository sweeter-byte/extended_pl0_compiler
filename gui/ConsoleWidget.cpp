#include "ConsoleWidget.h"
#include <QVBoxLayout>
#include <QFont>

ConsoleWidget::ConsoleWidget(QWidget *parent)
    : QWidget(parent), inputReady_(false)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // Output area
    outputArea_ = new QTextEdit(this);
    outputArea_->setReadOnly(true);
    
    // Dark theme
    QPalette p = outputArea_->palette();
    p.setColor(QPalette::Base, QColor("#1E1E1E"));
    p.setColor(QPalette::Text, QColor("#D4D4D4"));
    outputArea_->setPalette(p);
    
    QFont font("Consolas", 12);
    if (!QFontInfo(font).fixedPitch()) {
        font.setFamily("Monospace");
    }
    outputArea_->setFont(font);

    // Input line
    inputLine_ = new QLineEdit(this);
    inputLine_->setPlaceholderText("Enter input here...");
    inputLine_->setStyleSheet(
        "QLineEdit { background-color: #2D2D2D; color: #D4D4D4; "
        "border: 1px solid #3C3C3C; padding: 4px; }"
    );
    inputLine_->setFont(font);

    layout->addWidget(outputArea_, 1);
    layout->addWidget(inputLine_);

    connect(inputLine_, &QLineEdit::returnPressed, this, &ConsoleWidget::onInputSubmitted);
}

void ConsoleWidget::appendOutput(const QString& text)
{
    outputArea_->setTextColor(QColor("#4EC9B0"));  // Cyan
    outputArea_->append(text);
}

void ConsoleWidget::appendError(const QString& text)
{
    outputArea_->setTextColor(QColor("#F44747"));  // Red
    outputArea_->append(text);
}

void ConsoleWidget::appendInfo(const QString& text)
{
    outputArea_->setTextColor(QColor("#858585"));  // Gray
    outputArea_->append(text);
}

void ConsoleWidget::clear()
{
    outputArea_->clear();
    inputLine_->clear();
    pendingInput_.clear();
    inputReady_ = false;
}

void ConsoleWidget::onInputSubmitted()
{
    pendingInput_ = inputLine_->text();
    inputReady_ = true;
    
    outputArea_->setTextColor(QColor("#CE9178"));  // Orange
    outputArea_->append("> " + pendingInput_);
    inputLine_->clear();
    
    Q_EMIT inputSubmitted(pendingInput_);
}

bool ConsoleWidget::hasInput() const
{
    return inputReady_;
}

QString ConsoleWidget::getInput()
{
    inputReady_ = false;
    QString input = pendingInput_;
    pendingInput_.clear();
    return input;
}

QString ConsoleWidget::waitForInput()
{
    // Note: This is a simplified version. Real implementation would need event loop handling.
    return pendingInput_;
}
