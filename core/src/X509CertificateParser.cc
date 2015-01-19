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

#include <X509CertificateParser.h>
#include <qcc/Crypto.h>
#include <qcc/CryptoECC.h>

#define OID_X509_OUNIT_NAME "2.5.4.11"

namespace ajn {
namespace securitymgr {
qcc::String X509CertificateParser::GetSerialNumber(qcc::String certificate)
{
    size_t first = certificate.find_first_of('\n', 0);
    size_t second = certificate.find_last_of('\n', certificate.length() - 10);
    qcc::String base64 = certificate.substr(first + 1, second - first - 1);
    qcc::String binary;
    qcc::Crypto_ASN1::DecodeBase64(base64, binary);

    qcc::String oid;
    qcc::String sig;
    qcc::String tmp;
    size_t siglen;
    qcc::Crypto_ASN1::Decode(binary, "((.)(o)b)", &tmp, &oid, &sig, &siglen);

    qcc::String tbs;
    qcc::Crypto_ASN1::Encode(tbs, "(R)", &tmp);

    uint32_t x509Version;
    qcc::String serial;
    qcc::String iss;
    qcc::String sub;
    qcc::String time;
    qcc::String pub;
    qcc::String ext;

    qcc::Crypto_ASN1::Decode(tbs, "(c(i)l(.)(.)(.)(.)(.).)",
                             0, &x509Version, &serial, &oid, &iss, &time, &sub, &pub, &ext);

    return serial;
}

qcc::GUID128 X509CertificateParser::GetGuildID(qcc::String certificate)
{
    size_t first = certificate.find_first_of('\n', 0);
    size_t second = certificate.find_last_of('\n', certificate.length() - 10);
    qcc::String base64 = certificate.substr(first + 1, second - first - 1);
    qcc::String binary;
    qcc::Crypto_ASN1::DecodeBase64(base64, binary);

    qcc::String rawOID;
    qcc::String oid = qcc::String(OID_X509_OUNIT_NAME);
    qcc::Crypto_ASN1::Encode(rawOID, "o", &oid);

    first = binary.find(rawOID) + rawOID.length();
    second = binary.find_first_of('0', first);
    qcc::String asnGuild = binary.substr(first);
    qcc::String rawGuildID;

    qcc::Crypto_ASN1::Decode(asnGuild, "u", &rawGuildID);

    qcc::GUID128 guildID(rawGuildID);
    return guildID;
}
}
}
