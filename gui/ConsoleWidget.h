#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>

class ConsoleWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConsoleWidget(QWidget *parent = nullptr);
    
    void appendOutput(const QString& text);
    void appendError(const QString& text);
    void appendInfo(const QString& text);
    void clear();
    
    QString waitForInput();
    bool hasInput() const;
    QString getInput();

Q_SIGNALS:
    void inputSubmitted(const QString& text);

private Q_SLOTS:
    void onInputSubmitted();

private:
    QTextEdit* outputArea_;
    QLineEdit* inputLine_;
    QString pendingInput_;
    bool inputReady_;
};

#endif // CONSOLEWIDGET_H
