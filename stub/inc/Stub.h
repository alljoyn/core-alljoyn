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

#ifndef STUB_H_
#define STUB_H_

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include "PermissionMgmt.h"
#include "MySessionListener.h"
#include <memory>

using namespace ajn;
using namespace services;

class Stub {
  public:
    Stub(ClaimListener* cl);
    virtual ~Stub();

    QStatus OpenClaimWindow();

    QStatus CloseClaimWindow();

    BusAttachment& GetBusAttachment()
    {
        return ba;
    }

    qcc::String GetInstalledIdentityCertificate() const
    {
        return pm->GetInstalledIdentityCertificate();
    }

    void GetUsedManifest(AuthorizationData& manifest) const
    {
        pm->GetUsedManifest(manifest);
    }

    void SetUsedManifest(const AuthorizationData& manifest)
    {
        pm->SetUsedManifest(manifest);
    }

    std::vector<qcc::ECCPublicKey*> GetRoTKeys() const
    {
        return pm->GetRoTKeys();
    }

    /* Not really clear why you need this */
    QStatus SendClaimDataSignal()
    {
        return pm->SendClaimDataSignal();
    }

    std::map<qcc::String, qcc::String> GetMembershipCertificates() const;

  private:
    BusAttachment ba;
    MySessionListener ml;
    PermissionMgmt* pm;

    QStatus AdvertiseApplication(const char* guid);

    QStatus StartListeningForSession(TransportMask mask);

    QStatus StopListeningForSession();

    QStatus FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore,
                                           const char* guid);
};

#endif /* STUB_H_ */
