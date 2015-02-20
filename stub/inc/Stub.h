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

#ifndef STUB_H_
#define STUB_H_

#define APPLICATION_PORT 3333

#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/SessionPortListener.h>
#include <alljoyn/about/AboutPropertyStoreImpl.h>

#include "PermissionMgmt.h"
#include <memory>

using namespace ajn;
using namespace services;

class Stub {
  public:
    Stub(ClaimListener* cl,
         bool dsa = false);
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

    void GetUsedManifest(PermissionPolicy::Rule* manifestRules,
                         size_t manifestRulesCount) const
    {
        pm->GetUsedManifest(manifestRules, manifestRulesCount);
    }

    void SetUsedManifest(PermissionPolicy::Rule* manifestRules,
                         size_t manifestRulesCount)
    {
        ba.GetPermissionConfigurator().SetPermissionManifest(manifestRules, manifestRulesCount);
        pm->SetUsedManifest(manifestRules, manifestRulesCount);
    }

    std::vector<qcc::ECCPublicKey*> GetRoTKeys() const
    {
        return pm->GetRoTKeys();
    }

    void SetDSASecurity(bool dsa);

    /* Not really clear why you need this */
    QStatus SendClaimDataSignal()
    {
        return pm->SendClaimDataSignal();
    }

    std::map<GUID128, qcc::String> GetMembershipCertificates() const;

    QStatus Reset();

  private:
    BusAttachment ba;
    PermissionMgmt* pm;

    QStatus AdvertiseApplication(const char* guid);

    QStatus FillAboutPropertyStoreImplData(AboutPropertyStoreImpl* propStore,
                                           const char* guid);

    QStatus GenerateManifest(PermissionPolicy::Rule** retRules,
                             size_t* count);
};

#endif /* STUB_H_ */
