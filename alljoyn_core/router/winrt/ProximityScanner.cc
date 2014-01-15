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

#include <qcc/platform.h>
#include <map>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <utility>
#include <qcc/winrt/utility.h>
#include <collection.h>
#include <Objbase.h>
#include "ProximityScanner.h"

#define QCC_MODULE "PROXIMITY_SCANNER"

using namespace qcc;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;


namespace ajn {

ProximityScanner::ProximityScanner(BusAttachment& bus) : bus(bus) {
    QCC_DbgTrace(("ProximityScanner::ProximityScanner()"));
}

void ProximityScanner::PrintBSSIDMap(std::map<qcc::String, qcc::String> mymap) {

    std::map<qcc::String, qcc::String>::iterator it;
    for (it = mymap.begin(); it != mymap.end(); it++) {
        //QCC_DbgPrintf(("\n BSSID : %s", it->first.c_str()));
    }
}

#define MAX_GUID_STRING_SIZE 39

void ProximityScanner::Scan(bool request_scan) {
    QCC_DbgTrace(("ProximityScanner::Scan()"));

    // Start with a clean slate
    //
    scanResults.clear();

    IVectorView<HostName ^> ^ localEntries = NetworkInformation::GetHostNames();
    std::map<Platform::String ^, Platform::String ^> adapterMap;
    std::for_each(begin(localEntries), end(localEntries), [this, &adapterMap](HostName ^ entry) {
                      if (entry->IPInformation != nullptr && entry->IPInformation->NetworkAdapter != nullptr) {
                          auto op = entry->IPInformation->NetworkAdapter->GetConnectedProfileAsync();
                          concurrency::task<ConnectionProfile ^> getProfileTask(op);
                          try {
                              getProfileTask.wait();
                              ConnectionProfile ^ profile = getProfileTask.get();
                              if (profile != nullptr) {
                                  Platform::String ^ ssid = profile->ProfileName;

                                  WCHAR networkAdapterId[MAX_GUID_STRING_SIZE];
                                  Platform::String ^ networkAdapterIdStr = nullptr;
                                  if (StringFromGUID2(profile->NetworkAdapter->NetworkAdapterId, networkAdapterId, ARRAYSIZE(networkAdapterId))) {
                                      networkAdapterIdStr = ref new Platform::String(networkAdapterId);
                                  } else {
                                      QCC_LogError(ER_FAIL, ("Fail to convert GUID to String"));
                                      return;
                                  }
                                  adapterMap[networkAdapterIdStr] = ssid;
                                  QCC_DbgPrintf(("Connected network = %s, NetworkAdapterId =%s", PlatformToMultibyteString(ssid).c_str(), \
                                                 PlatformToMultibyteString(networkAdapterIdStr).c_str()));
                              }
                          } catch (Platform::Exception ^ e) {
                              QCC_LogError(ER_FAIL, ("Fail to get network connection profile."));
                              return;
                          }
                      }
                  });

    if (adapterMap.size() == 0) {
        QCC_DbgPrintf(("This device has no network connection established."));
        return;
    }

    IVectorView<LanIdentifier ^> ^ lanIdentifiers = nullptr;
    try {
        lanIdentifiers = NetworkInformation::GetLanIdentifiers();
    } catch (Platform::Exception ^ e) {
        QCC_LogError(ER_OS_ERROR, ("ProximityScanner::Scan() AccessDeniedException: The 'Location' capability should be enabled for the Application to access the LanIdentifier (location information)"));
        // AccessDeniedException will be threw if Location capability is not enabled for the Application
        return;
    }

    QCC_DbgPrintf(("The number of found LanIdentifiers = %d", lanIdentifiers->Size));
    std::for_each(begin(lanIdentifiers), end(lanIdentifiers), [this, adapterMap](LanIdentifier ^ lanIdentifier) {
                      WCHAR networkAdapterId[MAX_GUID_STRING_SIZE];
                      if (StringFromGUID2(lanIdentifier->NetworkAdapterId, networkAdapterId, ARRAYSIZE(networkAdapterId))) {
                          Platform::String ^ networkAdapterIdStr = ref new Platform::String(networkAdapterId);
                          QCC_DbgPrintf(("LandIdentifier's NetworkAdapterId = %s", PlatformToMultibyteString(networkAdapterIdStr).c_str()));
                          auto it = adapterMap.find(networkAdapterIdStr);
                          if (it != adapterMap.end()) {
                              QCC_DbgPrintf(("Find matched NetworkAdapterId = %s", PlatformToMultibyteString(networkAdapterIdStr).c_str()));
                              qcc::String bssid;
                              auto lanIdVals = lanIdentifier->InfrastructureId->Value;
                              if (lanIdVals->Size != 0) {
                                  bool first = true;
                                  std::for_each(begin(lanIdVals), end(lanIdVals), [this, &first, &bssid](int lanIdVal)
                                                {
                                                    if (!first) {
                                                        bssid.append(':');
                                                    } else {
                                                        first = false;
                                                    }
                                                    // represent the BSSID in lower case
                                                    qcc::String tmpStr = U32ToString(lanIdVal, 16, 2, '0');
                                                    std::for_each(tmpStr.begin(), tmpStr.end(), [&bssid](int c) {
                                                                      bssid.append(tolower(c));
                                                                  });
                                                });
                                  qcc::String ssid = PlatformToMultibyteString(it->second);
                                  scanResults.insert(std::map<std::pair<qcc::String, qcc::String>, bool>::value_type(std::make_pair(bssid, ssid), true));
                                  QCC_DbgPrintf(("Report scan result ssid = %s bssid = %s", ssid.c_str(), bssid.c_str()));
                                  return;
                              } else {
                                  QCC_DbgPrintf(("LanIdentifier's size = %d is too small", lanIdVals->Size));
                              }
                          } else {
                              QCC_DbgPrintf(("The NetworkAdapterId (%s) does not match any network connection", PlatformToMultibyteString(networkAdapterIdStr).c_str()));
                          }
                      }
                  });
}

}
