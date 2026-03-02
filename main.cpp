#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle("Fusion");

    QPalette dark;
    dark.setColor(QPalette::Window, QColor(30, 30, 30));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Base, QColor(20, 20, 20));
    dark.setColor(QPalette::Text, Qt::white);
    app.setPalette(dark);

    MainWindow w;
    w.show();
    return app.exec();
}