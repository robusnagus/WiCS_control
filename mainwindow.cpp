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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#define APP_NAME "Wireless Command Station 191112.1"

MainWindow::MainWindow(QWidget *parent)
          : QMainWindow(parent),
            ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(APP_NAME);

    retryCount = DEF_MAX_RETRY;
    cfgRetryMax = DEF_MAX_RETRY;
    cfgDgramTout = DEF_TOUT_DGRAM;
    cfgUpgradeTout = DEF_TOUT_UPGRADE;

    statConn = new QLabel(tr("Łączenie..."), this);
    statConn->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addWidget(statConn);
    statStatus = new QLabel(QString("--"), this);
    statStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statStatus->setMinimumWidth(150);
    statusBar()->addWidget(statStatus);

    thNet = new NetEngine(this);
    connect(thNet, SIGNAL(connected(quint16)),
            this, SLOT(networkConnected(quint16)));
    connect(thNet, SIGNAL(configinfo(quint16, QString)),
            this, SLOT(updateConfigInfo(quint16, QString)));
    connect(thNet, SIGNAL(imageopened(QString, qint64)),
            this, SLOT(imageOpened(QString, qint64)));
    connect(thNet, SIGNAL(upgradeinit(int)),
            this, SLOT(upgradeInit(int)));
    connect(thNet, SIGNAL(upgradestep(quint16, quint16)),
            this, SLOT(updateUpgradeStat(quint16, quint16)));

    timerNet = new QTimer(this);
    timerNet->setSingleShot(true);

    ui->cboxUpgModule->addItem(tr("Moduł WLAN"));
    ui->cboxUpgModule->addItem(tr("Moduł DCC"));
    ui->cboxUpgModule->setCurrentIndex(0);

    ui->cboxDevAddress->addItem(QHostAddress(getNIaddress() | 0x000000FF)
                                    .toString());
    ui->cboxDevAddress->setCurrentIndex(0);  

    ui->btnDevConnect->setEnabled(false);
    ui->btnDevClose->setEnabled(false);
    ui->grpDevInfo->setEnabled(false);
    controlEnable();

    thNet->openSocket(DEF_LAN_PORTNUM);

}  // MainWindow::MainWindow

MainWindow::~MainWindow()
{
    delete ui;
}

// aktywność kontrolek
void MainWindow::controlEnable()
{
    bool fEnable = ui->btnDevClose->isEnabled();

    ui->cboxDevAddress->setEnabled(ui->btnDevConnect->isEnabled());
    ui->grpDevWifi->setEnabled(fEnable);
    ui->grpDevOper->setEnabled(fEnable);

    ui->edUpgFilename->setEnabled(fEnable);
    fEnable = fEnable && (ui->pbarUpgrade->value() == 0);
    ui->btnDevRead->setEnabled(fEnable);
    ui->btnDevApply->setEnabled(fEnable);
    ui->grpDevWifi->setEnabled(fEnable);
    ui->btnUpgFile->setEnabled(fEnable);
    ui->cboxUpgModule->setEnabled(fEnable);
    ui->btnUpgStart->setEnabled(fEnable &&
                                !ui->edUpgFilename->text().isEmpty());

} // MainWindow::controlEnable

// zerowanie pól danych
void MainWindow::clearDevInfo()
{
    ui->labInfoHW->setText(QString("--"));
    ui->labInfoSW->setText(QString("--"));
    ui->labInfoFW->setText(QString("--"));
    ui->labInfoSN->setText(QString("--"));
    ui->edDevSsid->clear();
    ui->edDevPass->clear();

} // MainWindow::clearDevInfo

void MainWindow::clearUpgFilename()
{
    ui->edUpgFilename->clear();
    ui->labUpgStatus->setText(tr("Wybierz plik firmware"));
}

// adres interfejsu sieciowego
quint32 MainWindow::getNIaddress()
{
    QList<QHostAddress> addrList = QNetworkInterface::allAddresses();
    QHostAddress haddr;
    qint8 cnt;
    for (cnt = 0; cnt < addrList.count(); cnt++) {
        haddr = addrList.at(cnt);
        if ((haddr.protocol() == QAbstractSocket::IPv4Protocol)
            && !haddr.isLoopback())
            return haddr.toIPv4Address();
    }

    return 0xFFFFFFFF;

} // MainWindow::getNIaddress

// obsługa ogłoszenia podłączenia do portu
void MainWindow::networkConnected(quint16 port)
{
    if (port > 0) {
        ui->btnDevConnect->setEnabled(true);
        controlEnable();
        statConn->setText(tr("Port: %1").arg(port));
    }
    else {
        ui->btnDevConnect->setEnabled(false);
        ui->btnDevClose->setEnabled(false);
        controlEnable();
        statConn->setText(tr("Nie połączono"));
    }

} // MainWindow::networkConnected

void MainWindow::updateConfigInfo(quint16 opcode, QString data)
{
    QStringList citems = data.split(";");
    switch (opcode) {
    case WICS_WIFISTA:
        // informacje o podłączeniu do sieci
        if (citems.count() == 2) {
            ui->edDevSsid->setText(citems.at(0));
            ui->edDevPass->setText(citems.at(1));
        }
        break;
    case WICS_DEVINFO:
        // aktualizacja informacji o urządzeniu
        timerNet->stop();
        if (citems.count() == 5) {
            statConn->setText(tr("Połączono z %1").arg(citems.at(0)));
            ui->grpDevInfo->setEnabled(true);
            ui->labInfoHW->setText(citems.at(1));
            ui->labInfoSW->setText(citems.at(2));
            ui->labInfoFW->setText(citems.at(3));
            ui->labInfoSN->setText(citems.at(4));
            ui->btnDevClose->setEnabled(true);
            controlEnable();
        }
        break;
    default:
        qDebug("ConfigInfo zły opcode: %d", opcode);
        break;
    } // switch opcode

} // MainWindow::updateConfigInfo

// dane otwartego pliku
void MainWindow::imageOpened(QString iname, qint64 isize)
{
    if (isize != 0) {
        ui->edUpgFilename->setText(iname);
        ui->labUpgStatus->setText(tr("Rozmiar firmware: %1B").arg(isize));
    }
    else {
        ui->edUpgFilename->clear();
        ui->labUpgStatus->setText(tr("Błąd otwarcia pliku: %1").arg(iname));
    }
    controlEnable();

} // MainWindow::imageOpened

// inicjalizacja kontrolek aktualizacji
void MainWindow::upgradeInit(int steps)
{
    ui->pbarUpgrade->setMaximum(steps);
    ui->pbarUpgrade->setValue(0);
}

// wysłanie żądania komunikacji z urządzeniem
void MainWindow::findDevice()
{
    statConn->setText(tr("Szukanie urządzenia...."));   
    timerNet->disconnect(SIGNAL(timeout()));
    connect(timerNet, SIGNAL(timeout()), this, SLOT(findDeviceTout()));
    timerNet->start(static_cast<int>(cfgDgramTout * 2));
    thNet->sendDevInfoReq(QHostAddress(ui->cboxDevAddress->currentText())
                            .toIPv4Address());

} // MainWindow::findDevice

// przeterminowanie żądania komunikacji z urządzeniem
void MainWindow::findDeviceTout()
{
    qDebug("findDevice tout");
    statConn->setText(tr("Nie połączono"));
    ui->btnDevConnect->setEnabled(true);
    controlEnable();
}

// urządzenie nie odpowiada
void MainWindow::deviceNoAnswwer()
{
    ui->labUpgStatus->setText("Urządzenie nie odpowiada");
    ui->btnDevClose->setEnabled(true);
    controlEnable();
}

// przetermnowanie wiadomosci: start aktualizacji
void MainWindow::upgradeInitTout()
{
    qDebug("upgradeInit tout %d", retryCount);
    if (--retryCount) {
        // ponowienie
        timerNet->disconnect(SIGNAL(timeout()));
        connect(timerNet, SIGNAL(timeout()), this, SLOT(upgradeInitTout()));
        timerNet->start(static_cast<int>(cfgUpgradeTout));
        thNet->sendUpgradeInit(ui->cboxUpgModule->currentIndex() + UPGRADE_WLAN);
    }
    else {
        // limit prób wyczerpany
        deviceNoAnswwer();
    }

} // MainWindow::upgradeInitTout

// przeterminowanie wiadomości: dane aktualizacji
void MainWindow::upgradeDataTout()
{
    qDebug("upgradeData tout %d", retryCount);
    if (--retryCount) {
        // ponowienie
        timerNet->disconnect(SIGNAL(timeout()));
        connect(timerNet, SIGNAL(timeout()), this, SLOT(upgradeDataTout()));
        timerNet->start(static_cast<int>(cfgDgramTout * 3));
        thNet->sendUpgradeData();
    }
    else {
        // limit prób wyczerpany
        deviceNoAnswwer();
    }

} // MainWindow::upgradeDataTout

// klawisz Podłącz
void MainWindow::on_btnDevConnect_clicked()
{
    clearUpgFilename();
    ui->btnDevConnect->setEnabled(false);
    controlEnable();
    findDevice();
}

// klawisz Rozłącz
void MainWindow::on_btnDevClose_clicked()
{
    clearDevInfo();
    ui->btnDevConnect->setEnabled(true);
    ui->btnDevClose->setEnabled(false);
    controlEnable();
}

// klawisz Wczytaj
void MainWindow::on_btnDevRead_clicked()
{
    ui->edDevSsid->clear();
    ui->edDevPass->clear();
    thNet->sendWiFiStaReq();
}

// klawisz Zastosuj
void MainWindow::on_btnDevApply_clicked()
{
    thNet->sendWiFiSta(ui->edDevSsid->text(), ui->edDevPass->text());
}

// zmiana wyboru na liście modułów
void MainWindow::on_cboxUpgModule_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    clearUpgFilename();
    controlEnable();
}

// klawisz Wybierz (plik aktualizacji)
void MainWindow::on_btnUpgFile_clicked()
{
    QString imageCaption = tr("Wybierz program: ");
    QString imageFilter = tr("Plik z oprogramowaniem (*.bin);;"
                             "Wszystkie pliki (*.*)");
    int module = UPGRADE_WLAN + ui->cboxUpgModule->currentIndex();
    switch (module) {
    case UPGRADE_WLAN:
        imageCaption += tr("moduł WLAN"); break;
    case UPGRADE_DCCGEN:
        imageCaption += tr("moduł DCCG"); break;
    default:
        return;
    } // switch module
    ui->pbarUpgrade->setValue(0);
    QString imageName = QFileDialog::getOpenFileName(this, imageCaption,
                            QString(), imageFilter);

    clearUpgFilename();
    controlEnable();
    if (!imageName.isEmpty()) {
        thNet->openImageFile(imageName);
    }   

} // MainWindow::on_btnUpgFile_clicked

// klawisz Aktualizuj
void MainWindow::on_btnUpgStart_clicked()
{
    ui->btnDevClose->setEnabled(false);
    controlEnable();
    retryCount = cfgRetryMax;
    timerNet->disconnect(SIGNAL(timeout()));
    connect(timerNet, SIGNAL(timeout()), this, SLOT(upgradeInitTout()));
    ui->labUpgStatus->setText(tr("Uruchomienie aktualizacji"));
    statStatus->setText(tr("Aktualizacja oprogramowania"));
    timerNet->start(static_cast<int>(cfgUpgradeTout));
    thNet->sendUpgradeInit(ui->cboxUpgModule->currentIndex() + UPGRADE_WLAN);

} // MainWindow::on_btnUpgStart_clicked

// aktualizacja stanu ładowania firmware
void MainWindow::updateUpgradeStat(quint16 block, quint16 result)
{
    if (ui->pbarUpgrade->value() != static_cast<int>(block)) {
        ui->labUpgStatus->setText(tr("Potwierdzenie bloku %1, oczekiwany %2")
                                  .arg(block).arg(ui->pbarUpgrade->value()));
        qDebug("Blok upgrade: %d zamiast %d", block, ui->pbarUpgrade->value());
        return;
    }

    timerNet->disconnect(SIGNAL(timeout()));
    timerNet->stop();
    if (result == RESULT_OK) {
        qDebug("Upgrade stat: %d", block);
        // OK, wysłanie następnego bloku
        if (ui->pbarUpgrade->value() == ui->pbarUpgrade->maximum()) {
            // zakończenie
            ui->labUpgStatus->setText(tr("Aktualizacja zakończona"));
            statStatus->setText(tr("Oprogramowanie zaktualizowane"));
        }
        else {
            // następny blok
            ui->pbarUpgrade->setValue(ui->pbarUpgrade->value() + 1);
            thNet->sendUpgradeNewData();
            timerNet->disconnect(SIGNAL(timeout()));
            retryCount = cfgRetryMax;
            connect(timerNet, SIGNAL(timeout()), this, SLOT(upgradeDataTout()));
            timerNet->start(static_cast<int>(cfgUpgradeTout * 2));
        }
    }
    else {
        // błąd, przerwanie aktualizacji
        ui->labUpgStatus->setText(tr("Wystąpił błąd podczas aktualizacji, blok: %1")
                                  .arg(block));
    }

} // MainWindow::updateUpgradeStat

// EOF mainwindow.cpp
