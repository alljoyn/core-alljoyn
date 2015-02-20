/******************************************************************************
 * Copyright (c) 2015, AllSeen Alliance. All rights reserved.
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

#include <vector>

#include <qcc/GUID.h>
#include <qcc/CryptoECC.h>
#include <qcc/String.h>

#include <alljoyn/PermissionConfigurator.h>

#include <alljoyn/securitymgr/ApplicationState.h>

namespace ajn {
namespace securitymgr {
/*
 * \brief ApplicationInfo represents an application. Its primary key is its
 * public key.
 */
struct ApplicationInfo {
  public:
    /**
     * \brief The public key of the remote application. It is the unique
     * key to identify an application.
     */
    qcc::ECCPublicKey publicKey;
    /**
     * \brief The current or last known bus name of this application. Can be
     * empty if a claimed application was not discovered since the security
     * manager was started.
     */
    qcc::String busName;
    /**
     * \brief A user defied name for this application. This name is persisted
     * in local storage.
     */
    qcc::String userDefinedName;
    /**
     * \brief The name of the device on which the application is running, as
     * published in its about signal. When no such signal is received, the
     * name is equal to the empty string.
     */
    qcc::String deviceName;
    /**
     * \brief The name of the application as published in its about signal.
     * When no such signal is received, the name is equal to the empty string.
     */
    qcc::String appName;

    /**
     * \brief A list of public keys of security managers that have the right
     * to change the security configuration of this application.
     */
    std::vector<qcc::ECCPublicKey> rootsOfTrust;

    /**
     * \brief The ID of an application.
     */
    qcc::GUID128 peerID;

    /**
     * \brief The claim state of the application. An application can only be
     * claimed if it is in the CLAIMABLE state, and can only be managed by a
     * security manager if it is is in the CLAIMED state.
     */
    ajn::PermissionConfigurator::ClaimableState claimState;
    /**
     * \brief The running state of an application. Both RUNNING and NOT_RUNNING
     * application can be managed by a security manager.
     */
    ApplicationRunningState runningState;

    bool operator==(const ApplicationInfo& ai) const
    {
        if (publicKey != ai.publicKey) {
            return false;
        }

        return true;
    }
};
}
}

#endif /* APPLICATIONINFO_H_ */
