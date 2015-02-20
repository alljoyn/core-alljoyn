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

#ifndef SECURITYINFO_H_
#define SECURITYINFO_H_

#include <vector>
#include <alljoyn/about/AnnounceHandler.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>
#include <qcc/String.h>
#include <qcc/GUID.h>
#include <Common.h>
#include <alljoyn/securitymgr/ApplicationInfo.h>

#define QCC_MODULE "SEC_MGR"

namespace ajn {
namespace securitymgr {
/*
 * \brief Represents the data of a remote NotifyConfig signal.
 */
struct SecurityInfo {
  public:
    qcc::String busName;
    qcc::ECCPublicKey publicKey;
    ajn::PermissionConfigurator::ClaimableState claimState;
    uint32_t policySerialNum;
    ApplicationRunningState runningState;
    std::vector<qcc::ECCPublicKey> rootsOfTrust;

    SecurityInfo() { }

    SecurityInfo(const ApplicationInfo& appInfo) :
        busName(appInfo.busName),
        publicKey(appInfo.publicKey),
        claimState(appInfo.claimState),
        runningState(appInfo.runningState)
    {
    }
};
}
}
#undef QCC_MODULE
#endif /* SECURITYINFO_H_ */
