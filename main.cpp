//
// Project: Wireless Command Station
// File:    main.cpp
// Author:  Nagus
// Version: 20190823
//

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();

} // main

// EOF main.cpp
