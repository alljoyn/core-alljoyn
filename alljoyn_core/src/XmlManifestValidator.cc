/**
 * @file
 * This file defines the XmlManifestValidator for reading Security 2.0
 * policies from an XML.
 */

/******************************************************************************
 *  *    Copyright (c) Open Connectivity Foundation (OCF) and AllJoyn Open
 *    Source Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *     THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *     WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *     AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *     DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *     PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *     TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *     PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include <qcc/CertificateECC.h>
#include "XmlManifestValidator.h"
#include "XmlRulesValidator.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

QStatus XmlManifestValidator::Validate(const XmlElement* manifestXml)
{
    QCC_DbgHLPrintf(("%s: Validating signed manifest XML:\n%s.",
                     __FUNCTION__, manifestXml->Generate().c_str()));

    QStatus status = ValidateElementName(manifestXml, MANIFEST_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountEqual(manifestXml, MANIFEST_ROOT_ELEMENT_CHILDREN_COUNT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateManifestVersion(manifestXml->GetChildren()[MANIFEST_VERSION_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateRules(manifestXml->GetChildren()[MANIFEST_RULES_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateManifestThumbprint(manifestXml->GetChildren()[MANIFEST_THUMBPRINT_INDEX]);
    if (ER_OK != status) {
        return status;
    }

    return ValidateManifestSignature(manifestXml->GetChildren()[MANIFEST_SIGNATURE_INDEX]);
}

QStatus XmlManifestValidator::ValidateManifestVersion(const XmlElement* manifestVersion)
{
    QStatus status = ValidateElementName(manifestVersion, MANIFEST_VERSION_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidateManifestVersionContent(manifestVersion);
}

QStatus XmlManifestValidator::ValidateManifestVersionContent(const XmlElement* manifestVersion)
{
    uint32_t version = StringToU32(manifestVersion->GetContent());
    if (VALID_MANIFEST_VERSION_NUMBER != version) {
        QCC_LogError(ER_XML_INVALID_MANIFEST_VERSION,
                     ("%s: Invalid signed manifest version. Expected: %u. Was: %u.",
                      __FUNCTION__, VALID_MANIFEST_VERSION_NUMBER, version));

        return ER_XML_INVALID_MANIFEST_VERSION;
    }

    return ER_OK;
}

QStatus XmlManifestValidator::ValidateRules(const XmlElement* rules)
{
    QStatus status = ValidateElementName(rules, RULES_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return XmlRulesValidator::GetInstance()->Validate(rules);
}

QStatus XmlManifestValidator::ValidateManifestThumbprint(const XmlElement* thumbprint)
{
    QStatus status = ValidateElementName(thumbprint, THUMBPRINT_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountEqual(thumbprint, THUMBPRINT_ELEMENT_CHILDREN_COUNT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateOid(thumbprint->GetChildren()[OID_ELEMENT_INDEX], OID_DIG_SHA256.c_str());
    if (ER_OK != status) {
        return status;
    }

    return ValidateValueElement(thumbprint->GetChildren()[VALUE_ELEMENT_INDEX]);
}

QStatus XmlManifestValidator::ValidateManifestSignature(const XmlElement* manifestSignature)
{
    QStatus status = ValidateElementName(manifestSignature, SIGNATURE_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateChildrenCountEqual(manifestSignature, SIGNATURE_ELEMENT_CHILDER_COUNT);
    if (ER_OK != status) {
        return status;
    }

    status = ValidateOid(manifestSignature->GetChildren()[OID_ELEMENT_INDEX], OID_SIG_ECDSA_SHA256.c_str());
    if (ER_OK != status) {
        return status;
    }

    return ValidateValueElement(manifestSignature->GetChildren()[VALUE_ELEMENT_INDEX]);
}

QStatus XmlManifestValidator::ValidateOid(const XmlElement* xmlOid, AJ_PCSTR oid)
{
    QStatus status = ValidateElementName(xmlOid, OID_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidateOidContent(xmlOid, oid);
}

QStatus XmlManifestValidator::ValidateOidContent(const XmlElement* xmlOid, AJ_PCSTR oid)
{
    String xmlOidString = xmlOid->GetContent();
    if (xmlOidString != oid) {
        QCC_LogError(ER_XML_INVALID_OID,
                     ("%s: Invalid OID value. Expected: %s. Was: %s.",
                      __FUNCTION__, oid, xmlOidString.c_str()));

        return ER_XML_INVALID_OID;
    }

    return ER_OK;
}

QStatus XmlManifestValidator::ValidateValueElement(const XmlElement* xmlValue)
{
    QStatus status = ValidateElementName(xmlValue, VALUE_XML_ELEMENT);
    if (ER_OK != status) {
        return status;
    }

    return ValidateBase64Value(xmlValue->GetContent());
}

QStatus XmlManifestValidator::ValidateBase64Value(const String& value)
{
    vector<uint8_t> decodedValue;
    QStatus status = Crypto_ASN1::DecodeBase64(value, decodedValue);

    if (ER_OK != status) {
        QCC_LogError(ER_XML_INVALID_BASE64,
                     ("%s: Invalid base64 value: %s.",
                      __FUNCTION__, value.c_str()));

        return ER_XML_INVALID_BASE64;
    }

    return status;
}
} /* namespace ajn */