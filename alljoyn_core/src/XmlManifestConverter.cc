/**
 * @file
 * This file defines the XmlManifestConverter for reading Security 2.0
 * signed manifests from an XML.
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

#include <qcc/CertificateECC.h>
#include <qcc/Crypto.h>
#include <qcc/StringUtil.h>
#include "XmlManifestConverter.h"
#include "XmlManifestValidator.h"
#include "XmlRulesConverter.h"
#include "XmlRulesValidator.h"

using namespace qcc;
using namespace std;

namespace ajn {

QStatus XmlManifestConverter::XmlToManifest(AJ_PCSTR manifestXml, Manifest& manifest)
{
    QCC_ASSERT(nullptr != manifestXml);
    QCC_ASSERT(nullptr != &manifest);

    XmlElement* root = nullptr;
    QStatus status = XmlElement::GetRoot(manifestXml, &root);

    if (ER_OK == status) {
        status = XmlManifestValidator::Validate(root);
    }

    if (ER_OK == status) {
        BuildManifest(root, manifest);
    }

    delete root;
    return status;
}

QStatus XmlManifestConverter::ManifestToXml(const Manifest& manifest, string& manifestXml)
{
    QStatus status = XmlRulesValidator::ValidateRules(manifest->GetRules().data(), manifest->GetRules().size());

    if (ER_OK == status) {
        BuildManifest(manifest, manifestXml);
    }

    return status;
}

void XmlManifestConverter::BuildManifest(const XmlElement* root, Manifest& manifest)
{
    SetRules(root->GetChildren()[MANIFEST_RULES_INDEX], manifest);
    SetThumbprint(root->GetChildren()[MANIFEST_THUMBPRINT_INDEX], manifest);
    SetSignature(root->GetChildren()[MANIFEST_SIGNATURE_INDEX], manifest);
}

void XmlManifestConverter::SetRules(const XmlElement* rulesXml, Manifest& manifest)
{
    vector<PermissionPolicy::Rule> rules;

    QCC_VERIFY(ER_OK == XmlRulesConverter::XmlToRules(rulesXml->Generate().c_str(), rules));

    manifest->SetRules(rules.data(), rules.size());
}

void XmlManifestConverter::SetThumbprint(const XmlElement* thumbprintXml, Manifest& manifest)
{
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
    manifestElement->CreateChild(MANIFEST_VERSION_XML_ELEMENT)->AddContent(U32ToString(manifest->GetVersion()));
}

void XmlManifestConverter::BuildRules(const Manifest& manifest, XmlElement* manifestElement)
{
    XmlElement* rulesXml = nullptr;
    QCC_VERIFY(ER_OK == XmlRulesConverter::RulesToXml(manifest->GetRules().data(),
                                                      manifest->GetRules().size(),
                                                      &rulesXml,
                                                      RULES_XML_ELEMENT));
    manifestElement->AddChild(rulesXml);
}

void XmlManifestConverter::BuildThumbprint(const Manifest& manifest, XmlElement* manifestElement)
{
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
