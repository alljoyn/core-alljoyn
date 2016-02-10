/**
 * @file
 * This file defines the XmlManifestValidator for reading Security 2.0
 * policies from an XML.
 */

/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include <qcc/CertificateECC.h>
#include "XmlManifestValidator.h"
#include "XmlRulesValidator.h"

using namespace qcc;

namespace ajn {

QStatus XmlManifestValidator::Validate(const XmlElement* manifestXml)
{
    QStatus status = ValidateElementName(manifestXml, MANIFEST_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountEqual(manifestXml, MANIFEST_ROOT_ELEMENT_CHILDREN_COUNT);
    }

    if (ER_OK == status) {
        status = ValidateManifestVersion(manifestXml->GetChildren()[MANIFEST_VERSION_INDEX]);
    }

    if (ER_OK == status) {
        status = ValidateRules(manifestXml->GetChildren()[MANIFEST_RULES_INDEX]);
    }

    if (ER_OK == status) {
        status = ValidateManifestThumbprint(manifestXml->GetChildren()[MANIFEST_THUMBPRINT_INDEX]);
    }

    if (ER_OK == status) {
        status = ValidateManifestSignature(manifestXml->GetChildren()[MANIFEST_SIGNATURE_INDEX]);
    }

    return (ER_OK == status) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlManifestValidator::ValidateManifestVersion(const XmlElement* manifestVersion)
{
    QStatus status = ValidateElementName(manifestVersion, MANIFEST_VERSION_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateManifestVersionContent(manifestVersion);
    }

    return status;
}

QStatus XmlManifestValidator::ValidateManifestVersionContent(const XmlElement* manifestVersion)
{
    uint32_t version = StringToU32(manifestVersion->GetContent());
    return (VALID_MANIFEST_VERSION_NUMBER == version) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlManifestValidator::ValidateRules(const XmlElement* rules)
{
    QStatus status = ValidateElementName(rules, RULES_XML_ELEMENT);

    if (ER_OK == status) {
        status = XmlRulesValidator::Validate(rules);
    }

    return status;
}

QStatus XmlManifestValidator::ValidateManifestThumbprint(const XmlElement* thumbprint)
{
    QStatus status = ValidateElementName(thumbprint, THUMBPRINT_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountEqual(thumbprint, THUMBPRINT_ELEMENT_CHILDREN_COUNT);
    }

    if (ER_OK == status) {
        status = ValidateOid(thumbprint->GetChildren()[OID_ELEMENT_INDEX], OID_DIG_SHA256.c_str());
    }

    if (ER_OK == status) {
        status = ValidateValueElement(thumbprint->GetChildren()[VALUE_ELEMENT_INDEX]);
    }

    return status;
}

QStatus XmlManifestValidator::ValidateManifestSignature(const XmlElement* manifestSignature)
{
    QStatus status = ValidateElementName(manifestSignature, SIGNATURE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateChildrenCountEqual(manifestSignature, SIGNATURE_ELEMENT_CHILDER_COUNT);
    }

    if (ER_OK == status) {
        status = ValidateOid(manifestSignature->GetChildren()[OID_ELEMENT_INDEX], OID_SIG_ECDSA_SHA256.c_str());
    }

    if (ER_OK == status) {
        status = ValidateValueElement(manifestSignature->GetChildren()[VALUE_ELEMENT_INDEX]);
    }

    return status;
}

QStatus XmlManifestValidator::ValidateOid(const XmlElement* xmlOid, AJ_PCSTR oid)
{
    QStatus status = ValidateElementName(xmlOid, OID_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateOidContent(xmlOid, oid);
    }

    return status;
}

QStatus XmlManifestValidator::ValidateOidContent(const XmlElement* xmlOid, AJ_PCSTR oid)
{
    return (xmlOid->GetContent() == oid) ? ER_OK : ER_XML_MALFORMED;
}

QStatus XmlManifestValidator::ValidateValueElement(const XmlElement* xmlValue)
{
    QStatus status = ValidateElementName(xmlValue, VALUE_XML_ELEMENT);

    if (ER_OK == status) {
        status = ValidateBase64Value(xmlValue->GetContent());
    }

    return status;
}

QStatus XmlManifestValidator::ValidateBase64Value(const String& value)
{
    std::vector<uint8_t> decodedValue;
    return Crypto_ASN1::DecodeBase64(value, decodedValue);
}
} /* namespace ajn */