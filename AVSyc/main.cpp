#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
int main(int argc, char *argv[])
{
	CreateMutexA(0, FALSE, "Local\\$myprogram$");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return -1;
    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
