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

#include "netengine.h"

NetEngine::NetEngine(QObject *parent)
         : QThread(parent)
{
    thePort = 0;
    targetAddr = 0;
}

NetEngine::~NetEngine()
{
    closeSocket();
    wait();
}

void NetEngine::openSocket(quint16 theport)
{
    if (theport == 0) {
        closeSocket();
    }
    else {
        mutex.lock();
        thePort = theport;
        mutex.unlock();
        if (!isRunning()) {
            start();
        }
    }
} // NetEngine::openSocket

void NetEngine::closeSocket()
{
    mutex.lock();
    thePort = 0;
    mutex.unlock();
}

void NetEngine::run()
{
    QUdpSocket udp;
    qint64     bytes;
    quint32    address;
    QByteArray datagram;

    // otwarcie portu
    if (thePort > 0) {
        if (udp.bind(thePort)) {
            emit connected(thePort);
        }
        else {
            emit connected(0);
            thePort = 0;
        }
    }
    else {
        emit connected(0);
    }

    while (thePort != 0) {

        // wysłanie pakietów
        mutex.lock();
        QHashIterator<quint32, QByteArray> i(outBuffer);

        while (i.hasNext()) {
            i.next();
            address = i.key();
            datagram = i.value();
            bytes = udp.writeDatagram(datagram, QHostAddress(address), thePort);
            if (bytes == -1) {

            }
        } // hasNext

        outBuffer.clear();
        mutex.unlock();

        // odbiór pakietów
        while (udp.hasPendingDatagrams()) {
            quint16      senderPort;
            QHostAddress senderAddr;
            //datagram.clear();
            bytes = udp.pendingDatagramSize();
            qDebug("UDP datagram: %dB", static_cast<int>(bytes));
            datagram.resize(static_cast<int>(bytes));
            if (-1 != udp.readDatagram(datagram.data(), datagram.size(),
                                       &senderAddr, &senderPort)) {
                qDebug("%s", datagram.toHex().constData());
                processDatagram(senderAddr.toIPv4Address(), datagram);
            }
        }

    } // główna pętla

} // NetEngine::run

// przetwarzanie odebranego datagramu
void NetEngine::processDatagram(quint32 addr, const QByteArray& datagram)
{
    const NetDatagram_dg *data =
            reinterpret_cast<const NetDatagram_dg*>(datagram.constData());
    if (data->header != LAN_WICS_MESSAGE) {
        qDebug("Nieznany header: %X04 od %s", data->header,
               QHostAddress(addr).toString().toLatin1().data());
        qDebug("%s", datagram.toHex().constData());
        return;
    }

    switch (data->opcode) {
    // informacje o urządzeniu
    case WICS_DEVINFO:
        emitDevInfo(QHostAddress(addr).toString(),
                    reinterpret_cast<const DeviceInfo_dg*>(datagram.constData()));
        targetAddr = addr;
        sendWiFiStaReq();
        break;
    // informacje o podłączeniu do sieci
    case WICS_WIFISTA:
        if (addr == targetAddr) {
            emitWiFiSta(reinterpret_cast<const WiFiStation_dg*>
                        (datagram.constData()));
        }
        break;
    // stan aktualizacji
    case WICS_UPGRADE:
        if (addr == targetAddr) {
            emitUpgradeStep(reinterpret_cast<const UpgradeState_dg*>
                            (datagram.constData()));
        }
        else {
            qDebug("Upgrade od %s",
                   QHostAddress(addr).toString().toLatin1().data());
        }
        break;
    // nie rozpoznany datagram
    default:
        qDebug("Nieznany opcode: %X04 od %s\n%s", data->opcode,
               QHostAddress(addr).toString().toLatin1().data(),
               datagram.toHex().data());
        break;
    } // switch data.header

} // NetEngine::processDatagram

void NetEngine::emitDevInfo(QString addr, const DeviceInfo_dg* data)
{
    QStringList citems;
    citems << addr
           << QString("%1.%2")
                .arg(data->hwVersion / 100)
                .arg(data->hwVersion % 100)
           << QString("%1.%2.%3")
                .arg((data->swVersion >> 24) & 0xFF)
                .arg((data->swVersion >> 16) & 0xFF)
                .arg(data->swVersion & 0xFFFF)
           << QString("%1.%2.%3")
                .arg((data->fwVersion >> 24) & 0xFF)
                .arg((data->fwVersion >> 16) & 0xFF)
                .arg(data->fwVersion & 0xFFFF)
           << QString("%1")
                .arg(data->serialNum, 8, 16, QChar('0'));
    emit configinfo(WICS_DEVINFO, citems.join(";"));
} // NetEngine::emitDevInfo

void NetEngine::emitWiFiSta(const WiFiStation_dg* data)
{
    QStringList citems;
    citems << QString(data->ssid)
           << QString(data->pass);
    emit configinfo(WICS_WIFISTA, citems.join(";"));
}

void NetEngine::emitUpgradeStep(const UpgradeState_dg* data)
{
    emit upgradestep(data->block, data->result);
}

void NetEngine::openImageFile(QString filename)
{
    imageFile.setFileName(filename);
    if (imageFile.open(QIODevice::ReadOnly)) {
        // plik z oprogramowaniem otwarty
        emit imageopened(imageFile.fileName(), imageFile.size());
    }
    else {
        // błąd otwarcia pliku
        emit imageopened(imageFile.fileName(), 0);
    }

} // NetEngine::openImageFile

// wysłanie żądania informacji o urządzeniu
void NetEngine::sendDevInfoReq(quint32 targetaddr)
{
    static NetDatagram_dg data;
    data.bytes = static_cast<quint16>(sizeof(NetDatagram_dg));
    data.header = LAN_WICS_MESSAGE;
    data.opcode = WICS_DEVINFO_GET;
    data.param = WICS_PARAM_NONE;

    targetAddr = targetaddr;
    mutex.lock();
    outBuffer.insert(targetAddr, QByteArray::fromRawData
                     (reinterpret_cast<char*>(&data), sizeof(NetDatagram_dg)));
    mutex.unlock();

} // NetEngine::sendDevInfoReq

// wysłanie żądania danych połączenia WiFi
void NetEngine::sendWiFiStaReq()
{
    static NetDatagram_dg data;

    data.bytes = static_cast<quint16>(sizeof(NetDatagram_dg));
    data.header = LAN_WICS_MESSAGE;
    data.opcode = WICS_WIFISTA_GET;
    data.param = WICS_PARAM_NONE;

    mutex.lock();
    outBuffer.insert(targetAddr, QByteArray::fromRawData
                     (reinterpret_cast<char*>(&data), sizeof(NetDatagram_dg)));
    mutex.unlock();

} // NetEngine::sendWiFiStaRequest

// wysłanie danych połączenia WiFi
void NetEngine::sendWiFiSta(QString ssid, QString pass)
{
    static WiFiStation_dg data;
    data.bytes  = static_cast<quint16>(sizeof(WiFiStation_dg));
    data.header = LAN_WICS_MESSAGE;
    data.opcode = WICS_WIFISTA;

    strncpy(data.ssid, ssid.toLocal8Bit().data(), MAX_WLAN_NAME);
    strncpy(data.pass, pass.toLocal8Bit().data(), MAX_WLAN_PASS);

    mutex.lock();
    outBuffer.insert(targetAddr, QByteArray::fromRawData(
                     reinterpret_cast<char*>(&data), sizeof(WiFiStation_dg)));
    mutex.unlock();

} // NetEngine::sendWiFiSta

// wysłanie wiadomości: start aktualizacji
void NetEngine::sendUpgradeInit(int module)
{
    static UpgradeInit_dg data;
    data.bytes = static_cast<quint16>(sizeof(UpgradeInit_dg));
    data.header = LAN_WICS_MESSAGE;
    data.opcode = WICS_UPGRADE_START;
    data.fwsize = static_cast<quint32>(imageFile.size());

    switch (module) {
    case UPGRADE_WLAN:
        data.flags = UPGRADE_WLAN;
        imageBSize = UPG_WLAN_PAGE;
        break;
    case UPGRADE_DCCGEN:
        data.flags = UPGRADE_DCCGEN;
        imageBSize = UPG_DCCG_PAGE;
        break;
    default:
        qDebug("Moduł: %d", module);
        data.flags = 0;
        imageBSize = 256;
        break;
    } // switch module

    imageBlock = 0;
    imageFlags = static_cast<quint16>(module);
    emit upgradeinit(static_cast<int>((imageFile.size() / imageBSize) + 1));
    mutex.lock();
    outBuffer.insert(targetAddr, QByteArray::fromRawData(
                     reinterpret_cast<char*>(&data), sizeof(UpgradeInit_dg)));
    mutex.unlock();

} // NetEngine::sendUpgradeInit

// wysłanie wiadomości: dane aktualizacji
void NetEngine::sendUpgradeData()
{
    if (imageData.size() == 0) {
        // wysłanie nowego bloku
        imageData = imageFile.read(imageBSize);
        imageBlock++;

        static UpgradeData_dg data;
        data.bytes  = static_cast<quint16>
                        (imageData.size()) + sizeof(UpgradeData_dg);
        data.header = LAN_WICS_MESSAGE;
        data.opcode = WICS_UPGRADE_DATA;
        data.flags  = imageFlags;
        data.block  = static_cast<quint16>(imageBlock);

        imageData = imageData.prepend(QByteArray::fromRawData(
                        reinterpret_cast<char*>(&data), sizeof(UpgradeData_dg)));
    }

    qDebug("Send block: %d, %dB", imageBlock, imageData.size());
    mutex.lock();
    outBuffer.insert(targetAddr, imageData);
    mutex.unlock();

} // NetEngine::sendUpgradeData

void NetEngine::sendUpgradeNewData()
{
    imageData.clear();
    sendUpgradeData();
}

// EOF netengine.cpp
