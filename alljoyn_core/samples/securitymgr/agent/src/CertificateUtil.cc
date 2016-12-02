/******************************************************************************
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 ******************************************************************************/

#include <alljoyn/securitymgr/CertificateUtil.h>

#include <qcc/Debug.h>
#include <qcc/time.h>

#define QCC_MODULE "SECMGR_AGENT"

using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
void CertificateUtil::SetValityPeriod(const uint64_t validityPeriod, CertificateX509& cert)
{
    CertificateX509::ValidPeriod period;
    uint64_t currentTime = GetEpochTimestamp() / 1000;
    period.validFrom = currentTime;
    period.validTo = period.validFrom +  validityPeriod;
    period.validFrom = period.validFrom - 3600;
    cert.SetValidity(&period);
}

QStatus CertificateUtil::ToIdentityCertificate(const Application& app,
                                               const IdentityInfo& idInfo,
                                               const uint64_t validityPeriod,
                                               IdentityCertificate& cert)
{
    QStatus status = ER_FAIL;
    if (!validityPeriod) {
        return status;
    }
    cert.SetAlias(idInfo.guid.ToString());
    cert.SetSubjectPublicKey(app.keyInfo.GetPublicKey());
    cert.SetSubjectOU((const uint8_t*)idInfo.name.data(), idInfo.name.size());
    cert.SetCA(false);
    status = SetSubjectName(cert, app);
    SetValityPeriod(validityPeriod, cert);

    return status;
}

QStatus CertificateUtil::ToMembershipCertificate(const Application& app,
                                                 const GroupInfo& groupInfo,
                                                 const uint64_t validityPeriod,
                                                 MembershipCertificate& cert)
{
    QStatus status = ER_FAIL;
    if (!validityPeriod) {
        return status;
    }
    cert.SetGuild(groupInfo.guid);
    cert.SetSubjectPublicKey(app.keyInfo.GetPublicKey());
    cert.SetCA(false);
    status = SetSubjectName(cert, app);
    SetValityPeriod(validityPeriod, cert);

    return status;
}

QStatus CertificateUtil::SetSubjectName(CertificateX509& cert, const Application& app)
{
    QStatus status = ER_OK;
    if (app.keyInfo.GetKeyIdLen()) {
        cert.SetSubjectCN(app.keyInfo.GetKeyId(), app.keyInfo.GetKeyIdLen());
    } else {
        String id;
        status = CertificateX509::GenerateAuthorityKeyId(app.keyInfo.GetPublicKey(), id);
        if (status != ER_OK) {
            return status;
        }
        cert.SetSubjectCN((uint8_t*)id.data(), id.size());
    }
    return status;
}
}
}
#undef QCC_MODULE