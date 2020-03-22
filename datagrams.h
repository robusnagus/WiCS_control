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

#ifndef DATAGRAMS_H
#define DATAGRAMS_H

#include <QtGlobal>

#define DEF_LAN_PORTNUM     21105
#define DEF_MAX_RETRY       3
#define DEF_TOUT_DGRAM      3000
#define DEF_TOUT_UPGRADE    30000

#define HW_NGS_WICS         0xDCC1

#define LAN_WICS_MESSAGE    0x38

#define WICS_DEVINFO_GET    0x49
#define WICS_WIFISTA_GET    0x57
#define WICS_UPGRADE_START  0x55
#define WICS_UPGRADE_DATA   0x48

#define WICS_DEVINFO        0x69
#define WICS_WIFISTA        0x77
#define WICS_UPGRADE        0x75

#define WICS_PARAM_NONE     0

#define UPGRADE_WLAN        0x01
#define UPGRADE_DCCGEN      0x02
#define UPGRADE_MODULE_MASK 0x0F

#define UPG_WLAN_PAGE       1024
#define UPG_DCCG_PAGE       256

#define RESULT_OK           0

#define MAX_WLAN_NAME       32
#define MAX_WLAN_PASS       32

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    quint16 param;
} NetDatagram_dg;

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    quint16 flags;
    quint16 hardware;
    quint16 hwVersion;
    quint32 swVersion;
    quint32 fwVersion;
    quint32 serialNum;
} DeviceInfo_dg;

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    quint16 flags;
    quint32 fwsize;
} UpgradeInit_dg;

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    quint16 flags;
    quint16 block;
} UpgradeData_dg;

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    quint16 block;
    quint16 result;
} UpgradeState_dg;

typedef struct __attribute__ ((packed)) {
    quint16 bytes;
    quint16 header;
    quint16 opcode;
    char    ssid[MAX_WLAN_NAME+1];
    char    pass[MAX_WLAN_PASS+1];
} WiFiStation_dg;

#endif // DATAGRAMS_H
