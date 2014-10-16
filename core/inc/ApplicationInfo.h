/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#ifndef APPLICATIONINFO_H_
#define APPLICATIONINFO_H_

#include <ApplicationState.h>
#include <AuthorizationData.h>
#include <PublicKey.h>
#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>
#include <qcc/String.h>

#define QCC_MODULE "SEC_MGR"
#include <vector>

namespace ajn {
namespace securitymgr {
/*
 * \brief Represents all information that is known about an application.
 *        It mixes information from the About signal, the SecInfo signal and the
 *        data persisted in local storage.
 */
struct ApplicationInfo {
  public:
    qcc::String busName; //When online. Does it need to be part of the public API?
    qcc::String userDefinedName;     //locally stored
    qcc::String deviceName;
    qcc::String appName;
    qcc::String appID;
    PublicKey publicKey;
    std::vector<PublicKey> rootOfTrustList;
    ApplicationClaimState claimState;
    ApplicationRunningState runningState;
    AuthorizationData manifest;
    qcc::String policy; //Human readable format - This has to change to PermissionPolicy once the latter has a copy constructor

    bool operator==(const ApplicationInfo& ai) const
    {
        if (busName != ai.busName) {
            return false;
        }

        if (userDefinedName != ai.userDefinedName) {
            return false;
        }

        if (deviceName != ai.deviceName) {
            return false;
        }

        if (appName != ai.appName) {
            return false;
        }

        if (appID != ai.appID) {
            return false;
        }

        if (publicKey != ai.publicKey) {
            return false;
        }

        if (claimState != ai.claimState) {
            return false;
        }

        if (runningState != ai.runningState) {
            return false;
        }
        if (policy != ai.policy) {
            return false;
        }

        return true;
    }
};
}
}
#undef QCC_MODULE
#endif /* APPLICATIONINFO_H_ */
