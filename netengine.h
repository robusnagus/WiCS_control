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

#ifndef NETENGINE_H
#define NETENGINE_H

#include <QThread>
#include <QMutex>
#include <QMultiHash>
#include <QtNetwork/QUdpSocket>
#include <QFile>

#include "datagrams.h"

class NetEngine : public QThread
{
    Q_OBJECT

private:
    quint16 thePort;
    QMutex mutex;
    QMultiHash<quint32, QByteArray> outBuffer;
    quint32     targetAddr;     // adres docelowy
    QFile       imageFile;      // plik firmware
    QByteArray  imageData;      // blok danych firmware
    quint16     imageBSize;     // rozmiar bloku danych
    int         imageBlock;     // numer bieżącego bloku
    quint16     imageFlags;

protected:
    void run();
protected:
    void processDatagram(quint32 addr, const QByteArray& datagram);
    void emitUpgradeStep(const UpgradeState_dg* data);
    void emitWiFiSta(const WiFiStation_dg* data);
    void emitDevInfo(QString addr, const DeviceInfo_dg* data);

public:
    explicit NetEngine(QObject *parent = nullptr);
    ~NetEngine();

signals:
    void connected(const quint16 port);
    void configinfo(quint16 opcode, QString data);
    void imageopened(QString iname, qint64 isize);
    void upgradeinit(int steps);
    void upgradestep(quint16 block, quint16 result);

public slots:
    void openSocket(quint16 theport);
    void closeSocket();
    void sendDevInfoReq(quint32 targetaddr);
    void sendWiFiStaReq();
    void sendWiFiSta(QString ssid, QString pass);
    void sendUpgradeInit(int module);
    void sendUpgradeData();
    void sendUpgradeNewData();
    void openImageFile(QString filename);

}; // NetEngine

#endif // NETENGINE_H
