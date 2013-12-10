/**
 * @file
 * ProximityScanner provides the scan results used by the Discovery framework and Rendezvous server
 */

/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#pragma comment(lib, "wlanapi.lib")

#include <qcc/platform.h>
#include <map>
#include <stdio.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/MsgArg.h>
#include <utility>
#include "ProximityScanner.h"
#include <windows.h>
#include <wlanapi.h>
#include <wchar.h>
#define QCC_MODULE "PROXIMITY_SCANNER"

using namespace qcc;

bool notification = false;

namespace ajn {

/* Callback function we registered to listen for WLAN events */
void WlanScanNotification(WLAN_NOTIFICATION_DATA*data, VOID*context) {
    QCC_DbgTrace(("ScanNotification()"));
    if (data->NotificationCode == wlan_notification_acm_scan_complete) {
        QCC_DbgPrintf(("\n Notification for SUCCESSFUL scanning received"));
    } else if (data->NotificationCode == wlan_notification_acm_scan_fail) {
        QCC_DbgPrintf(("\n Notification for FAILED scanning received"));
    }
    notification = true;
}

ProximityScanner::ProximityScanner(BusAttachment& bus) : bus(bus) {
    QCC_DbgTrace(("ProximityScanner::ProximityScanner()"));
}

void ProximityScanner::PrintBSSIDMap(std::map<qcc::String, qcc::String> mymap) {
    QCC_DbgTrace(("\n ProximityScanner::PrintBSSIDMap()"));
    std::map<qcc::String, qcc::String>::iterator it;
    for (it = mymap.begin(); it != mymap.end(); it++) {
        QCC_DbgPrintf(("\n BSSID : %s", it->first.c_str()));
    }

}


void ProximityScanner::Scan(bool request_scan) {

    QCC_DbgTrace(("\n Inside Scan()"));
    HANDLE wlan_scan_handle;
    GUID guid;
    DWORD dwError = 0;
    DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
    PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
    WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;

    char c_bssid[18];
    char c_ssid[255];
    bool attached = false;
    qcc::String bssid;
    qcc::String ssid;

    /* Version of WLAN API  */
    DWORD dwClientVersion = 2;

    /* Negotiated version of the WLAN API used */
    DWORD dwNegotiatedVersion = 0;

    dwError = WlanOpenHandle(dwClientVersion, NULL, &dwNegotiatedVersion, &wlan_scan_handle);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while creating a wlan handle"));
    } else {
        QCC_DbgPrintf(("\n Handle created successfully"));
    }

    WLAN_INTERFACE_INFO_LIST* wlanInforList = (WLAN_INTERFACE_INFO_LIST*)WlanAllocateMemory(sizeof(WLAN_INTERFACE_INFO_LIST));
    SecureZeroMemory(wlanInforList, sizeof(WLAN_INTERFACE_INFO_LIST));

    dwError = WlanEnumInterfaces(wlan_scan_handle, NULL, &wlanInforList);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while getting the WlanInfoList"));
    } else {
        QCC_DbgPrintf(("\n WlanInfoList obtained successfully"));
    }

    /* Before calling a scan we register a callback since the scan might not return immediately */
    DWORD dwPrevNotifSource = 0;
    dwError = WlanRegisterNotification(wlan_scan_handle, WLAN_NOTIFICATION_SOURCE_ALL, TRUE,   (WLAN_NOTIFICATION_CALLBACK)WlanScanNotification, NULL, NULL, &dwPrevNotifSource);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Could not register the ScanNotification callback\n"));
    } else {
        QCC_DbgPrintf(("\n ScanNotification callback registered successfully"));
    }
    /*
     *	Now we may have more than one wireless interface so we have to choose one
     *	For now we will choose the first one
     *	To call scan we need to guid of the interface we choose
     */
    if (wlanInforList->dwNumberOfItems  >= 1) {
        guid = wlanInforList->InterfaceInfo[0].InterfaceGuid;
    } else {
        QCC_DbgPrintf(("\n No Wireless interface is available to proceed further\n"));
    }
    /* We have everything to make a call to WlanScan */
    dwError = WlanScan(wlan_scan_handle, &guid, NULL, NULL, NULL);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while calling WlanScan function"));
    } else {
        QCC_DbgPrintf(("\n WlanScan function was called successfull"));
    }
    /* Need to come up with a better way of waiting for notifications rather than busy wait here */
    while (!notification) {
        QCC_DbgPrintf(("\n Waiting for notification"));
        qcc::Sleep(1000);
    }
    /* Set notification to false so that the next call to Scan waits for the updated results */
    notification = false;

    /* Now that we know whether our scan request went through or not we Unregister our callback so that is does not keep giving us notifications */
    dwError = WlanRegisterNotification(wlan_scan_handle, WLAN_NOTIFICATION_SOURCE_NONE, TRUE, NULL, NULL, NULL, &dwPrevNotifSource);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while unregistering notification callback"));
    } else {
        QCC_DbgPrintf(("\n ScanNotification callback was successfully unregistered"));
    }

    /* Now that we initiated a scan and even got a confirmation from the callback we get the list of available networks */
    WLAN_BSS_LIST*wlanBssidList = (WLAN_BSS_LIST*)WlanAllocateMemory(sizeof(WLAN_BSS_LIST));
    WLAN_AVAILABLE_NETWORK_LIST*wlanAvailableNetworkList = (WLAN_AVAILABLE_NETWORK_LIST*)WlanAllocateMemory(sizeof(WLAN_AVAILABLE_NETWORK_LIST));

    SecureZeroMemory(wlanBssidList, sizeof(WLAN_BSS_LIST));
    SecureZeroMemory(wlanAvailableNetworkList, sizeof(WLAN_AVAILABLE_NETWORK_LIST));

    dwError = WlanGetNetworkBssList(wlan_scan_handle, &guid, NULL, (DOT11_BSS_TYPE)dot11_BSS_type_any, NULL, NULL, &wlanBssidList);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while calling WlanGetNetworkBssList "));
    } else {
        QCC_DbgPrintf(("\n Call to WlanGetNetworkBssList SUCCESSFUL\n"));
    }

    /* First retrieve the network to which we are connected */
    dwError = WlanQueryInterface(wlan_scan_handle, &guid, wlan_intf_opcode_current_connection, NULL, &connectInfoSize, (PVOID*) &pConnectInfo, &opCode);
    if (ERROR_SUCCESS != dwError) {
        QCC_DbgPrintf(("\n Error while calling WlanQueryInterface"));
    } else {
        QCC_DbgPrintf(("\n Call to WlanQueryInterface SUCCESSFUL\n"));
    }
    char c_connected_network[18];
    snprintf(c_connected_network, 18, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", pConnectInfo->wlanAssociationAttributes.dot11Bssid[0],
             pConnectInfo->wlanAssociationAttributes.dot11Bssid[1],
             pConnectInfo->wlanAssociationAttributes.dot11Bssid[2],
             pConnectInfo->wlanAssociationAttributes.dot11Bssid[3],
             pConnectInfo->wlanAssociationAttributes.dot11Bssid[4],
             pConnectInfo->wlanAssociationAttributes.dot11Bssid[5]);

    /* Collect the BSSIDs and SSIDs. Clear the scan results map */
    scanResults.clear();

    /* Update the scan results map and print out the BSSIDs and SSID that we got from the function call above */
    for (int i = 0; i < wlanBssidList->dwNumberOfItems; i++) {

        snprintf(c_bssid, 18, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", wlanBssidList->wlanBssEntries[i].dot11Bssid[0],
                 wlanBssidList->wlanBssEntries[i].dot11Bssid[1],
                 wlanBssidList->wlanBssEntries[i].dot11Bssid[2],
                 wlanBssidList->wlanBssEntries[i].dot11Bssid[3],
                 wlanBssidList->wlanBssEntries[i].dot11Bssid[4],
                 wlanBssidList->wlanBssEntries[i].dot11Bssid[5]);
        qcc::String bssid(c_bssid);
        QCC_DbgPrintf(("BSSID - %s   ", bssid.c_str()));

        snprintf(c_ssid, 255, "%s", wlanBssidList->wlanBssEntries[i].dot11Ssid.ucSSID);
        qcc::String ssid(c_ssid);
        QCC_DbgPrintf(("SSID - %s \n ", ssid.c_str()));

        /* Check if this is the BSSID to which we are connected */
        attached = false;
        if (strcmp(c_connected_network, c_bssid) == 0) {
            QCC_DbgPrintf(("\n Found a BSSID match for connected network : %s", c_bssid));
            attached = true;
        }
        scanResults.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(bssid, ssid), attached));
    }
}
}