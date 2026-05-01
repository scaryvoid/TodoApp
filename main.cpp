#include <QApplication>
#include "MainWindow.h"
#include "TodoItem.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<TodoItem>("TodoItem");

    MainWindow w;
    w.resize(1100, 600);
    w.show();

    return app.exec();
}