/**
 * @file
 * This file defines the XmlManifestConverter for reading Security 2.0
 * signed manifests from an XML.
 */

/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
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
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include <qcc/CertificateECC.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include "XmlManifestConverter.h"
#include "XmlManifestValidator.h"
#include "XmlRulesConverter.h"
#include "XmlRulesValidator.h"

#define QCC_MODULE "XML_CONVERTER"

using namespace qcc;
using namespace std;

namespace ajn {

QStatus XmlManifestConverter::XmlToManifest(AJ_PCSTR manifestXml, Manifest& manifest)
{
    QCC_ASSERT(nullptr != manifestXml);
    QCC_ASSERT(nullptr != &manifest);

    QCC_DbgHLPrintf(("%s: Converting signed manifest XML into a Manifest object: %s",
                     __FUNCTION__, manifestXml));

    XmlElement* root = nullptr;
    QStatus status = XmlElement::GetRoot(manifestXml, &root);
    if (ER_OK != status) {
        return status;
    }

    status = XmlManifestValidator::Validate(root);
    if (ER_OK != status) {
        delete root;
        return status;
    }

    BuildManifest(root, manifest);

    delete root;
    return ER_OK;
}

QStatus XmlManifestConverter::ManifestToXml(const Manifest& manifest, string& manifestXml)
{
    QCC_DbgHLPrintf(("%s: Converting a Manifest object into a signed manifest XML: %s",
                     __FUNCTION__, manifest->ToString().c_str()));

    QStatus status = XmlRulesValidator::GetInstance()->ValidateRules(manifest->GetRules().data(), manifest->GetRules().size());
    if (ER_OK != status) {
        return status;
    }

    BuildManifest(manifest, manifestXml);

    return ER_OK;
}

QStatus XmlManifestConverter::XmlArrayToManifests(AJ_PCSTR* manifestsXmls, size_t manifestsCount, vector<Manifest>& manifests)
{
    QCC_DbgHLPrintf(("%s: Converting an array of %u signed manifest XMLs into Manifest objects",
                     __FUNCTION__, manifestsCount));

    manifests.resize(manifestsCount);
    for (size_t i = 0; i < manifestsCount; i++) {
        QStatus status = XmlToManifest(manifestsXmls[i], manifests[i]);
        if (ER_OK != status) {
            manifests.clear();
            return status;
        }
    }

    return ER_OK;
}

QStatus XmlManifestConverter::ManifestsToXmlArray(const Manifest* manifests, size_t manifestsCount, vector<string>& manifestsXmls)
{
    QCC_DbgHLPrintf(("%s: Converting an array of %u Manifest objects into signed manifest XMLs.",
                     __FUNCTION__, manifestsCount));

    manifestsXmls.resize(manifestsCount);
    for (size_t i = 0; i < manifestsCount; i++) {
        QStatus status = ManifestToXml(manifests[i], manifestsXmls[i]);
        if (ER_OK != status) {
            manifestsXmls.clear();
            return status;
        }
    }

    return ER_OK;
}

void XmlManifestConverter::BuildManifest(const XmlElement* root, Manifest& manifest)
{
    SetRules(root->GetChildren()[MANIFEST_RULES_INDEX], manifest);
    SetThumbprint(root->GetChildren()[MANIFEST_THUMBPRINT_INDEX], manifest);
    SetSignature(root->GetChildren()[MANIFEST_SIGNATURE_INDEX], manifest);
}

void XmlManifestConverter::SetRules(const XmlElement* rulesXml, Manifest& manifest)
{
    QCC_DbgTrace(("%s: Setting the manifest rules.", __FUNCTION__));
    vector<PermissionPolicy::Rule> rules;

    QCC_VERIFY(ER_OK == XmlRulesConverter::GetInstance()->XmlToRules(rulesXml->Generate().c_str(), rules));

    manifest->SetRules(rules.data(), rules.size());
}

void XmlManifestConverter::SetThumbprint(const XmlElement* thumbprintXml, Manifest& manifest)
{
    QCC_DbgTrace(("%s: Setting the manifest thumbprint.", __FUNCTION__));

    SetThumbprintOid(thumbprintXml, manifest);
    SetThumbprintValue(thumbprintXml, manifest);
}

void XmlManifestConverter::SetThumbprintOid(const XmlElement* thumbprintXml, Manifest& manifest)
{
    String thumbprintOid = thumbprintXml->GetChildren()[OID_ELEMENT_INDEX]->GetContent();
    manifest->m_thumbprintAlgorithmOid = thumbprintOid.c_str();
}

void XmlManifestConverter::SetThumbprintValue(const XmlElement* thumbprintXml, Manifest& manifest)
{
    String thumbprintValue = thumbprintXml->GetChildren()[VALUE_ELEMENT_INDEX]->GetContent();
    QCC_VERIFY(ER_OK == Crypto_ASN1::DecodeBase64(thumbprintValue, manifest->m_thumbprint));
}

void XmlManifestConverter::SetSignature(const XmlElement* signatureXml, Manifest& manifest)
{
    QCC_DbgTrace(("%s: Setting the manifest signature.", __FUNCTION__));

    SetSignatureOid(signatureXml, manifest);
    SetSignatureValue(signatureXml, manifest);
}

void XmlManifestConverter::SetSignatureOid(const XmlElement* signatureXml, Manifest& manifest)
{
    String signatureOid = signatureXml->GetChildren()[OID_ELEMENT_INDEX]->GetContent();
    manifest->m_signatureAlgorithmOid = signatureOid.c_str();
}

void XmlManifestConverter::SetSignatureValue(const XmlElement* signatureXml, Manifest& manifest)
{
    String signatureValue = signatureXml->GetChildren()[VALUE_ELEMENT_INDEX]->GetContent();
    QCC_VERIFY(ER_OK == Crypto_ASN1::DecodeBase64(signatureValue, manifest->m_signature));
}

void XmlManifestConverter::BuildManifest(const Manifest& manifest, string& manifestXml)
{
    XmlElement* root = new XmlElement(MANIFEST_XML_ELEMENT);

    BuildXmlManifestContents(manifest, root);
    manifestXml = root->Generate();

    delete root;
}

void XmlManifestConverter::BuildXmlManifestContents(const Manifest& manifest, XmlElement* manifestXml)
{
    BuildVersion(manifest, manifestXml);
    BuildRules(manifest, manifestXml);
    BuildThumbprint(manifest, manifestXml);
    BuildSignature(manifest, manifestXml);
}

void XmlManifestConverter::BuildVersion(const Manifest& manifest, XmlElement* manifestElement)
{
    QCC_DbgTrace(("%s: Setting the manifest XML version.", __FUNCTION__));

    manifestElement->CreateChild(MANIFEST_VERSION_XML_ELEMENT)->AddContent(U32ToString(manifest->GetVersion()));
}

void XmlManifestConverter::BuildRules(const Manifest& manifest, XmlElement* manifestElement)
{
    QCC_DbgTrace(("%s: Setting the manifest XML rules.", __FUNCTION__));

    XmlElement* rulesXml = nullptr;
    QCC_VERIFY(ER_OK == XmlRulesConverter::GetInstance()->RulesToXml(manifest->GetRules().data(),
                                                                     manifest->GetRules().size(),
                                                                     &rulesXml));
    manifestElement->AddChild(rulesXml);
}

void XmlManifestConverter::BuildThumbprint(const Manifest& manifest, XmlElement* manifestElement)
{
    QCC_DbgTrace(("%s: Setting the manifest XML thumbprint.", __FUNCTION__));

    XmlElement* thumbprintElement = manifestElement->CreateChild(THUMBPRINT_XML_ELEMENT);

    BuildThumbprintContent(manifest, thumbprintElement);
}

void XmlManifestConverter::BuildThumbprintContent(const Manifest& manifest, XmlElement* thumbprintElement)
{
    thumbprintElement->CreateChild(OID_XML_ELEMENT)->AddContent(manifest->GetThumbprintAlgorithmOid().c_str());
    BuildValue(manifest->GetThumbprint(), thumbprintElement);
}

void XmlManifestConverter::BuildSignature(const Manifest& manifest, XmlElement* manifestElement)
{
    QCC_DbgTrace(("%s: Setting the manifest XML signature.", __FUNCTION__));

    XmlElement* signatureElement = manifestElement->CreateChild(SIGNATURE_XML_ELEMENT);

    BuildSignatureContent(manifest, signatureElement);
}

void XmlManifestConverter::BuildSignatureContent(const Manifest& manifest, XmlElement* signatureElement)
{
    signatureElement->CreateChild(OID_XML_ELEMENT)->AddContent(manifest->GetSignatureAlgorithmOid().c_str());
    BuildValue(manifest->GetSignature(), signatureElement);
}

void XmlManifestConverter::BuildValue(const vector<uint8_t>& binaryValue, XmlElement* xmlElement)
{
    String base64Value;

    QCC_VERIFY(ER_OK == Crypto_ASN1::EncodeBase64(binaryValue, base64Value));

    /**
     * Removing '\n' at the end of the base64 string.
     */
    xmlElement->CreateChild(VALUE_XML_ELEMENT)->AddContent(base64Value.substr(0, base64Value.size() - 1));
}
} /* namespace ajn */
