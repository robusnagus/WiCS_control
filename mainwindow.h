//
// Wireless Command Station
//
// Copyright 2020 Robert Nagowski
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// See gpl-3.0.md file for details.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QFileDialog>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>

#include "datagrams.h"
#include "netengine.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Ui::MainWindow *ui;
    QLabel *statConn;
    QLabel *statStatus;
private:
    NetEngine *thNet;
    QTimer *timerNet;
private:
    quint8  retryCount;
    quint8  cfgRetryMax;
    quint32 cfgDgramTout;
    quint32 cfgUpgradeTout;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void controlEnable();
    void clearDevInfo();
    void clearUpgFilename();
    quint32 getNIaddress();
    void deviceNoAnswwer();
    void findDevice();
    void updateDevInfo(const DeviceInfo_dg *data);

private slots:
    void on_cboxUpgModule_currentIndexChanged(int index);
    void on_btnDevConnect_clicked();
    void on_btnDevClose_clicked();
    void on_btnDevRead_clicked();
    void on_btnDevApply_clicked();
    void on_btnUpgFile_clicked();
    void on_btnUpgStart_clicked();
    void findDeviceTout();
    void upgradeInitTout();
    void upgradeDataTout();

public slots:
    void networkConnected(quint16 port);
    void updateConfigInfo(quint16 opcode, QString data);
    void imageOpened(QString iname, qint64 isize);
    void upgradeInit(int steps);
    void updateUpgradeStat(quint16 block, quint16 result);

}; // MainWindow

#endif // MAINWINDOW_H
