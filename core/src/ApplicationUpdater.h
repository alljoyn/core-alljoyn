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

#ifndef APPLICATIONUPDATER_H_
#define APPLICATIONUPDATER_H_

#include <qcc/GUID.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/Status.h>

#include <X509CertificateGenerator.h>
#include <alljoyn/securitymgr/Storage.h>
#include <alljoyn/securitymgr/ApplicationInfo.h>

#include "RemoteApplicationManager.h"
#include "SecurityInfoListener.h"
#include "TaskQueue.h"

namespace ajn {
namespace securitymgr {
class SecurityEvent {
  public:
    SecurityInfo* newInfo;
    SecurityInfo* oldInfo;
    SecurityEvent(const SecurityInfo* n,
                  const SecurityInfo* o) :
        newInfo(n == NULL ? NULL :
                    new SecurityInfo(*n)), oldInfo(o == NULL ? NULL :
                                                       new SecurityInfo(*o))
    {
    }

    ~SecurityEvent()
    {
        delete newInfo;
        delete oldInfo;
    }
};

class ApplicationUpdater :
    public SecurityInfoListener {
  public:
    ApplicationUpdater(BusAttachment* ba, // no ownership
                       Storage* s, // no ownership
                       RemoteApplicationManager* ram, // no ownership
                       qcc::GUID128 guid,
                       ECCPublicKey pubkey) :
        busAttachment(ba), storage(s), applicationManager(ram),
        securityManagerGUID(guid), securityManagerPubkey(pubkey),
        queue(TaskQueue<SecurityEvent*, ApplicationUpdater>(this))
    {
        certificateGenerator = new X509CertificateGenerator(guid.ToString(), ba);
    }

    ~ApplicationUpdater()
    {
        queue.Stop();
    }

    QStatus UpdateApplication(const ApplicationInfo& appInfo);

    QStatus UpdateApplication(const SecurityInfo& secInfo);

    void HandleTask(SecurityEvent* event);

  private:
    QStatus UpdateApplication(const ApplicationInfo& appInfo,
                              const SecurityInfo& secInfo);

    QStatus ResetApplication(const ApplicationInfo& appInfo,
                             const SecurityInfo& secInfo);

    QStatus UpdatePolicy(const ApplicationInfo& appInfo,
                         const SecurityInfo& secInfo,
                         const ManagedApplicationInfo& mgdAppInfo);

    QStatus UpdateMembershipCertificates(const ApplicationInfo& appInfo,
                                         const SecurityInfo& secInfo,
                                         const ManagedApplicationInfo& mgdAppInfo);

    QStatus UpdateIdentityCert(const ApplicationInfo& appInfo,
                               const SecurityInfo& secInfo,
                               const ManagedApplicationInfo& mgdAppInfo);

    virtual void OnSecurityStateChange(const SecurityInfo* oldSecInfo,
                                       const SecurityInfo* newSecInfo);

  private:
    ajn::BusAttachment* busAttachment;
    Storage* storage;
    RemoteApplicationManager* applicationManager;
    qcc::GUID128 securityManagerGUID;
    ECCPublicKey securityManagerPubkey;
    TaskQueue<SecurityEvent*, ApplicationUpdater> queue;
    X509CertificateGenerator* certificateGenerator;
};
}
}

#endif /* APPLICATIONUPDATER_H_ */
