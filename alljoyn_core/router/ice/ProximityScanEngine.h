/**
 * @file
 * ProximityScanEngine is the control class that manages things after the scan interval expires
 * It manages the final map of BSSIDs that is used by the Discovery Manager and Rendezvous Server
 * The final map is obtained from the scan function which is platform specific and is implemented
 * by the class ProximityScanner which can be found in platform specific folders
 *
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
#ifndef _ALLJOYN_PROXIMITYSCANENGINE_H
#define _ALLJOYN_PROXIMITYSCANENGINE_H


#include <qcc/platform.h>
#include <map>
#include <qcc/Timer.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/Thread.h>

#include "DiscoveryManager.h"
#include "ProximityScanner.h"
#include <alljoyn/BusAttachment.h>

#define TADD_COUNT 4
#define TDROP_COUNT 4

const uint64_t SCAN_DELAY = 15000;
const int START_COUNT = 4;
//class ProximityScanner;

namespace ajn {

class ProximityScanEngine : public qcc::AlarmListener, public qcc::Thread {



  public:
    friend class ajn::ProximityScanner;

    /* Constructor for ProximityScanner */
    ProximityScanEngine(ajn::DiscoveryManager*dm);

    /* Function to get the Proximity Message containing the Final map of BSSIDs and also
     * return a sorted list of the BSSIDs and the BT MAC IDs*/
    ProximityMessage GetScanResults(list<String>& bssids, list<String>& macIds);

    /* Function to start scan and setup related timers */
    void StartScan();

    /* Function to stop scan and related timers */
    void StopScan();

    /* Function to request Scan */
    //void Scan();

    /* Function to process scan results */
    void ProcessScanResults();

    /* Function to print a BSSID map */
    void PrintFinalMap();

    /* Function to print Hysteresis map */
    void PrintHysteresis();

    /* Callback function called when Tscan expires */
    void AlarmTriggered(const qcc::Alarm& alarm, QStatus reason);

    /* Function to Add an alarm for the specified delay on the fly */
    void AddAlarm(uint32_t delay);

    /* Destructor */
    ~ProximityScanEngine();


    bool wifiapDroppped;
    //bool wifiapAdded;
    bool wifiON;
    bool request_scan;


    std::map<std::pair<qcc::String, qcc::String>, int>   hysteresisMap; /* Map used to keep track of BSSIDs for adding/removal from the final list */
    std::map<std::pair<qcc::String, qcc::String>, bool>  finalMap; /* The Map holding the final set sent to the Rendezvous */

    qcc::Mutex bssid_lock;                                      /* Mutex for initial_bssid and final_bssid */

    qcc::Timer mainTimer;                                       /* Timer to which all the alarms are added */

    int tadd_count;                                             /* tadd = 4 * tscan */
    int no_scan_results_count;

    DiscoveryManager* discoveryManager;                         /* Pointer to the instance of DiscoveryManager that calls ProximityScanner */

    ProximityScanner* proximityScanner;                         /* Object that implements the platform specific Scan function */
    BusAttachment& bus;

  private:

    /* Default constructor */
    ProximityScanEngine();

    /* Just defined to make klocwork happy. Should never be used */
    ProximityScanEngine(const ProximityScanEngine& other) :
        mainTimer("ProximityScanTimer"),
        bus(other.bus)
    {
        assert(false);
    }

    /* Just defined to make klocwork happy. Should never be used */
    ProximityScanEngine& operator=(const ProximityScanEngine& other) {
        assert(false);
        return *this;
    }

};


}
#endif
