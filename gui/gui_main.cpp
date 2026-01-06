#include "MainWindow.h"
#include <QApplication>

#include <clocale>

int main(int argc, char *argv[]) {
    std::setlocale(LC_ALL, "");
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName("PL/0 Compiler");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("PL0 Project");
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
