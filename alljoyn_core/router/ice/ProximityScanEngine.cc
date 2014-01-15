/**
 * @file
 * ProximityScanner gets the scan results used by the Discovery framework and Rendezvous server
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
#include <qcc/platform.h>
#include <map>
#include <qcc/Timer.h>
#include <stdio.h>
#include <qcc/Thread.h>


#include "DiscoveryManager.h"
#include "ProximityScanEngine.h"
#include "ProximityScanner.h"

using namespace std;
using namespace qcc;

#define QCC_MODULE "PROXIMITY_SCAN_ENGINE"
#define LOG_TAG  "ProximityScanEngine"

#if defined (QCC_OS_ANDROID)
#include <android/log.h>
        #ifndef LOGD
        #define LOGD(...) (__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
        #endif

        #ifndef LOGI
        #define LOGI(...) (__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
        #endif

        #ifndef LOGE
        #define LOGE(...) (__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
        #endif
#endif



class DiscoveryManager;

namespace ajn {


ProximityMessage ProximityScanEngine::GetScanResults(list<String>& bssids, list<String>& macIds) {

    QCC_DbgTrace(("ProximityScanEngine::GetScanResults() called"));
    ProximityMessage proximityMessage;
    list<WiFiProximity> wifi_bssid_list;
    list<BTProximity> bt_mac_list;
    std::map<std::pair<qcc::String, qcc::String>, bool>::iterator final_map_it;

    wifi_bssid_list.clear();
    bt_mac_list.clear();
    bssids.clear();
    macIds.clear();

    bssid_lock.Lock(MUTEX_CONTEXT);
    for (final_map_it = finalMap.begin(); final_map_it != finalMap.end(); final_map_it++) {
        WiFiProximity bssid_info;
        bssid_info.BSSID = final_map_it->first.first;
        bssid_info.SSID = final_map_it->first.second;
        bssid_info.attached = final_map_it->second;
        wifi_bssid_list.push_front(bssid_info);
        bssids.push_back(bssid_info.BSSID);
    }
    bssid_lock.Unlock(MUTEX_CONTEXT);
    proximityMessage.wifiaps = wifi_bssid_list;
    proximityMessage.BTs = bt_mac_list;

    /* Sort the lists */
    if (!bssids.empty()) {
        bssids.sort();
    }

    if (!macIds.empty()) {
        macIds.sort();
    }

    return proximityMessage;
}



void ProximityScanEngine::PrintFinalMap() {

    QCC_DbgTrace(("ProximityScanEngine::PrintFinalMap() called"));
    QCC_DbgPrintf(("-------------------Final Map ----------------------"));
    std::map<std::pair<qcc::String, qcc::String>, bool>::iterator it;
    for (it = finalMap.begin(); it != finalMap.end(); it++) {
        QCC_DbgPrintf(("BSSID: %s  SSID: %s attached: %s", it->first.first.c_str(), it->first.second.c_str(), it->second ? "true" : "false"));
    }
    QCC_DbgPrintf((" ---------------------------------------------------"));



}

void ProximityScanEngine::PrintHysteresis() {
    QCC_DbgTrace(("ProximityScanEngine::PrintHysteresis() called"));
    QCC_DbgPrintf(("-------------Hysteresis Map -----------------"));
    if (hysteresisMap.empty()) {
        QCC_DbgPrintf(("MAP is CLEAR"));
    }
    std::map<std::pair<qcc::String, qcc::String>, int>::iterator it;
    for (it = hysteresisMap.begin(); it != hysteresisMap.end(); it++) {
        QCC_DbgPrintf(("BSSID: %s   SSID: %s   COUNT: %d", it->first.first.c_str(), it->first.second.c_str(), it->second));
    }
    QCC_DbgPrintf(("----------------------------------------------"));
}

ProximityScanEngine::ProximityScanEngine(DiscoveryManager*dm) : mainTimer("ProximityScanTimer"), bus(dm->bus) {

    QCC_DbgTrace(("ProximityScanEngine::ProximityScanEngine() called"));
    tadd_count = 1;
    wifiapDroppped = false;
    //wifiapAdded = false;
    wifiON = false;
    //request_scan = false;
    request_scan = true;
    no_scan_results_count = 0;
    discoveryManager = dm;
    proximityScanner = new ProximityScanner(bus);
    finalMap.clear();
    hysteresisMap.clear();
}


ProximityScanEngine::~ProximityScanEngine() {
    QCC_DbgTrace(("ProximityScanEngine::~ProximityScanEngine() called"));

    StopScan();

    delete proximityScanner;
    proximityScanner = NULL;
}

void ProximityScanEngine::ProcessScanResults() {

    QCC_DbgTrace(("ProximityScanEngine::ProcessScanResults() called"));
    QCC_DbgPrintf(("Size of scan results = %d", proximityScanner->scanResults.size()));
    QCC_DbgPrintf(("Size of scan Hysteresis = %d", hysteresisMap.size()));
    QCC_DbgPrintf(("Size of scan Final Map = %d", finalMap.size()));

    std::map<std::pair<qcc::String, qcc::String>, bool>::iterator it;
    std::map<std::pair<qcc::String, qcc::String>, int>::iterator hit;

    // First get the scan results and update the hysteresis map : increase the count of the ones seen and
    // decrease the count of the ones not seen
    //
    // Increment count if present else add to Hysteresis AND final map
    //

    QCC_DbgPrintf(("Incrementing counts in the Hysteresis Map..."));
    QCC_DbgPrintf(("BEFORE Incrementing the maps are"));
    //PrintHysteresis();
    //PrintFinalMap();

    if (proximityScanner == NULL) {
        QCC_LogError(ER_FAIL, ("proximityScanner == NULL "));
    }

    bssid_lock.Lock(MUTEX_CONTEXT);
    for (it = proximityScanner->scanResults.begin(); it != proximityScanner->scanResults.end(); it++) {
        //hit = hysteresisMap.find(it->first);
        hit = hysteresisMap.find(std::pair<qcc::String, qcc::String>(it->first.first, it->first.second));
        if (hit != hysteresisMap.end()) {
            QCC_DbgPrintf(("Found the entry in hysteresisMap"));
            hit->second = START_COUNT;
            QCC_DbgPrintf(("Value of scan entry =%s,%s updated to %d", hit->first.first.c_str(), hit->first.second.c_str(), hit->second));
        } else {

            QCC_DbgPrintf(("Inserting new entry in the hysteresis and final map <%s,%s> , %s", it->first.first.c_str(), it->first.second.c_str(), it->second ? "true" : "false"));

            // Make pair outside
            hysteresisMap.insert(std::map<std::pair<qcc::String, qcc::String>, int>::value_type(std::make_pair(it->first.first, it->first.second), START_COUNT));
            finalMap.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(it->first.first, it->first.second), it->second));

        }
    }
    bssid_lock.Unlock(MUTEX_CONTEXT);

    if (proximityScanner->scanResults.size() > 0) {
        QCC_DbgPrintf(("Scan returned results so APs were added to the final Map"));
        //wifiapAdded = true;
        wifiON = true;
    }
    QCC_DbgPrintf(("Printing Maps after incrementing counts in Hysteresis Map"));
    PrintHysteresis();
    PrintFinalMap();

    //
    // Decrement count of those not present in scan results
    //
    //
    // look at the final hysteresis map
    // if count has reached zero remove it from the final AND hysteresis map in that order since you need the key from hysteresis
    //          Update final map .. Indicate with a boolean that there has been a change in the final map
    QCC_DbgPrintf(("Decrementing counts in Hysteresis Map "));
    bssid_lock.Lock(MUTEX_CONTEXT);
    hit = hysteresisMap.begin();
    while (hit != hysteresisMap.end()) {
        it = proximityScanner->scanResults.find(hit->first);
        if (it == proximityScanner->scanResults.end()) {
            hit->second = hit->second - 1;
            QCC_DbgPrintf(("Value of <%s,%s> = %d after decrementing", hit->first.first.c_str(), hit->first.second.c_str(), hit->second));
            if (hit->second == 0) {
                wifiapDroppped = true;
                QCC_DbgPrintf(("Entry <%s,%s> reached count 0 .... Deleting from hysteresis and final map", hit->first.first.c_str(), hit->first.second.c_str()));
                finalMap.erase(hit->first);
                hysteresisMap.erase(hit++);
            } else {
                ++hit;
            }
        } else {
            ++hit;
        }
    }
    bssid_lock.Unlock(MUTEX_CONTEXT);
//  We send an update in two conditions:
//	1. We reached Tadd count == 4 and the scan results are being returned with some results (non-empty)
//	2. Something was dropped from the final map

    PrintHysteresis();
    PrintFinalMap();

    // If TADD_COUNT has been reached
    //		all entries in the hysteresis with count > 0 make it to final
    //          Update final map .. Indicate with a boolean that there has been a change in the final map

    if ((tadd_count == TADD_COUNT && wifiON) || wifiapDroppped || request_scan) {
        // Form the proximity message if needed by checking for the boolean set in the above two cases
        // and Queue it if there is a change

        list<String> bssids;
        list<String> macIds;

        bssids.clear();
        macIds.clear();

        ProximityMessage proximityMsg = GetScanResults(bssids, macIds);
        QCC_DbgPrintf(("=-=-=-=-=-=-=-=-=-=-=-= Queuing Proximity Message =-=-=-=-=-=-=-=-=-=-=-="));
        PrintFinalMap();

        discoveryManager->QueueProximityMessage(proximityMsg, bssids, macIds);

        wifiapDroppped = false;
        //wifiapAdded = false;
        wifiON = true;
        tadd_count = 0;
    } else {
        tadd_count++;
    }

    //
    // This needs to checked for the following conditions:
    // 1. We did not get any opportunistic scan results since the last 4 scans = 60 secs.
    //    This could mean that we are either connected to a network in which case we are not returned any results
    //	  This could also mean that Wifi is turned off or the phone is acting as a portable hotspot
    // 2. Wifi is turned ON but we do not have any networks in the vicinity. In that case it is not harmful to
    //    request a scan once in 120 secs apart from what the device is already doing
    //

    if (proximityScanner->scanResults.size() <= 1) {
        no_scan_results_count++;
    } else {
        no_scan_results_count = 0;
    }


    if (no_scan_results_count == 3) {
        request_scan = true;
    } else {
        request_scan = false;
    }

}


void ProximityScanEngine::AlarmTriggered(const Alarm& alarm, QStatus reason)
{
    /*
     * We need to check (reason == ER_OK) here because we should not be
     * reloading the alarm if it has been triggered during the shutdown
     * of the timer.
     */
    if (reason == ER_OK) {
        while (true) {
            uint64_t start = GetTimestamp64();
            proximityScanner->Scan(request_scan);
            ProcessScanResults();
            uint32_t delay = ::max((uint64_t)0, SCAN_DELAY - (GetTimestamp64() - start));
            if (delay > 0) {
                //Add alarm with delay time to our main timer
                QCC_DbgPrintf(("Adding Alarm "));
                AddAlarm(delay);
                break;
            }
        }
    }
}

void ProximityScanEngine::StopScan() {

    QCC_DbgTrace(("ProximityScanEngine::StopScan() called"));
    //LOGD("============== ProximityScanEngine::StopScan() called ================");

    // Stop, RemoveAlarms and Join the mainTimer
    mainTimer.Stop();
    mainTimer.RemoveAlarmsWithListener(*this);
    mainTimer.Join();

    hysteresisMap.clear();
    finalMap.clear();
    //isFirstScanComplete = false;
    wifiapDroppped = false;
    //wifiapAdded = false;
    wifiON = false;
    request_scan = true;
    no_scan_results_count = 0;
    QCC_DbgPrintf(("ProximityScanEngine::StopScan() completed"));

}

void ProximityScanEngine::StartScan() {

    QCC_DbgTrace(("ProximityScanEngine::StartScan() called"));

    //request_scan = true;

    map<qcc::String, qcc::String>::iterator it;

    // Start the timer

    mainTimer.Start();

    // Add an immediate fire alarm to the timer
    uint32_t zero = 0;
    void* vptr = NULL;
    AlarmListener* myListener = this;
    mainTimer.AddAlarm(Alarm(zero, myListener, vptr, zero));

//    while (true) {
//        if (!isFirstScanComplete) {
//            QCC_DbgPrintf(("Sleeping before getting first scan results"));
//            qcc::Sleep(2000);
//        } else {
//            break;
//        }
//    }

}

// AddAlarm() which will tell the alarm about the listener. That listener should be made a part of the class definition of ProximityScanEngine

void ProximityScanEngine::AddAlarm(uint32_t delay) {

    uint32_t periodMs = 0;
    AlarmListener* myListener = this;
    void* vptr = NULL;
    Alarm tScan(delay, myListener, vptr, periodMs);
    mainTimer.AddAlarm(tScan);
}

}
