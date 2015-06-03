/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

#include <alljoyn/Status.h>
#include <CredentialAccessor.h>
#include <qcc/String.h>
#include <qcc/Debug.h>
#include <ctime>

#include <X509CertificateGenerator.h>

#define QCC_MODULE "SEC_MGR"

//using namespace ajn;
using namespace std;
using namespace qcc;

namespace ajn {
namespace securitymgr {
X509CertificateGenerator::X509CertificateGenerator(qcc::GUID128 issuerGUID,
                                                   ajn::BusAttachment* ba) :
    issuerGUID(issuerGUID), keys(new Crypto_ECC()), ba(ba)
{
    CredentialAccessor ca(*ba);
    ECCPublicKey pubKey;
    ca.GetDSAPublicKey(pubKey);
    keys->SetDSAPublicKey(&pubKey);
    ECCPrivateKey privKey;
    ca.GetDSAPrivateKey(privKey);
    keys->SetDSAPrivateKey(&privKey);
}

X509CertificateGenerator::~X509CertificateGenerator()
{
    delete keys;
}

QStatus X509CertificateGenerator::GenerateMembershipCertificate(qcc::MembershipCertificate& certificate)
{
    return GenerateDerEncodeCertificate(certificate);
}

QStatus X509CertificateGenerator::GetIdentityCertificate(qcc::IdentityCertificate& idCertificate)
{
    return GenerateDerEncodeCertificate(idCertificate);
}

QStatus X509CertificateGenerator::GenerateDerEncodeCertificate(qcc::CertificateX509& x509)
{
    PermissionConfigurator& pcf = ba->GetPermissionConfigurator();
    x509.SetIssuerCN(issuerGUID.GetBytes(), qcc::GUID128::SIZE);
    x509.GenerateAuthorityKeyId(keys->GetDSAPublicKey());
    QStatus status =  pcf.SignCertificate(x509);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to sign certificate"));
        return status;
    }
    qcc::String der;
    status = x509.EncodeCertificateDER(der);
    if (ER_OK != status) {
        QCC_LogError(status, ("Failed to encode certificate"));
        return status;
    }
    return status;
}
}
}

#undef QCC_MODULE
