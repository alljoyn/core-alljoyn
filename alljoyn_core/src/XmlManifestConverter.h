#ifndef _ALLJOYN_MANIFEST_XML_CONVERTER_H
#define _ALLJOYN_MANIFEST_XML_CONVERTER_H
/**
 * @file
 * This file defines the converter for Security 2.0 signed manifest in XML format.
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

#ifndef __cplusplus
#error Only include XmlManifestConverter.h in C++ code.
#endif

#include <alljoyn/PermissionPolicy.h>
#include <alljoyn/Status.h>
#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <string>

namespace ajn {

class XmlManifestConverter {
  public:

    /**
     * Extract manifest from an XML. The manifest XML schema
     * is available under alljoyn_core/docs/manifest.xsd.
     *
     * @param[in]    manifestXml   Manifest in XML format.
     * @param[out]   manifest      Reference to the new manifest.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_XML_MALFORMED if the XML does not follow the manifest XML schema.
     */
    static QStatus XmlToManifest(AJ_PCSTR manifestXml, Manifest& manifest);

    /**
     * Extract a manifest in XML format from a Manifest object.
     *
     * @param[in]    manifest      Input Manifest object.
     * @param[out]   manifestXml   Manifest in XML format.
     *
     * @return   #ER_OK if extracted correctly.
     *           #ER_FAIL if the manifest contains incorrect data.
     */
    static QStatus ManifestToXml(const Manifest& manifest, std::string& manifestXml);

  private:

    /**
     * Sets all of the Manifest values according to the input XML.
     *
     * @param[in]    root        Root element of the signed manifest XML.
     * @param[out]   manifest    Reference to the new manifest.
     */
    static void BuildManifest(const qcc::XmlElement* root, Manifest& manifest);

    /**
     * Set the Manifest's rules according to the input XML.
     *
     * @param[in]    rulesXml    The "rules" element of the signed manifest XML.
     * @param[out]   manifest    Reference to the new manifest.
     */
    static void SetRules(const qcc::XmlElement* rulesXml, Manifest& manifest);

    /**
     * Sets the Manifest's thumbprint according to the input XML.
     *
     * @param[in]    xmlThumbprint   The "thumbprint" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetThumbprint(const qcc::XmlElement* xmlThumbprint, Manifest& manifest);

    /**
     * Sets the Manifest's thumbprint OID according to the input XML.
     *
     * @param[in]    xmlThumbprint   The "thumbprint" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetThumbprintOid(const qcc::XmlElement* thumbprintXml, Manifest& manifest);

    /**
     * Sets the Manifest thumbprints "oid" element according to the input XML.
     *
     * @param[in]    xmlThumbprint   The "thumbprint" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetThumbprintValue(const qcc::XmlElement* thumbprint, Manifest& manifest);

    /**
     * Set the Manifest's signature according to the input XML.
     *
     * @param[in]    signatureXml    The "signature" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetSignature(const qcc::XmlElement* signatureXml, Manifest& manifest);

    /**
     * Set the Manifest's signature OID according to the input XML.
     *
     * @param[in]    signatureXml    The "signature" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetSignatureOid(const qcc::XmlElement* signatureXml, Manifest& manifest);

    /**
     * Sets the Manifest signature "oid" element according to the input XML.
     *
     * @param[in]    signatureXml    The "signature" element of the signed manifest XML.
     * @param[out]   manifest        Reference to the new manifest.
     */
    static void SetSignatureValue(const qcc::XmlElement* signatureXml, Manifest& manifest);

    /**
     * Builds a signed manifest in XML format from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildManifest(const Manifest& manifest, std::string& manifestXml);

    /**
     * Build the contents of a signed manifest in XML format from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildXmlManifestContents(const Manifest& manifest, qcc::XmlElement* manifestXml);

    /**
     * Builds a signed manifest's "version" element from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildVersion(const Manifest& manifest, qcc::XmlElement* manifestElement);

    /**
     * Builds a signed manifest's "rules" element from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildRules(const Manifest& manifest, qcc::XmlElement* manifestElement);

    /**
     * Builds a signed manifest's "thumbprint" element from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildThumbprint(const Manifest& manifest, qcc::XmlElement* manifestElement);

    /**
     * Builds a signed manifest's "thumbprint" element contents from a Manifest object.
     *
     * @param[in]    manifest            Reference to the input Manifest object.
     * @param[out]   thumbprintElement   The built signed manifest's "thumbprint" element.
     */
    static void BuildThumbprintContent(const Manifest& manifest, qcc::XmlElement* thumbprintElement);

    /**
     * Builds a signed manifest's "signature" element from a Manifest object.
     *
     * @param[in]    manifest    Reference to the input Manifest object.
     * @param[out]   manifestXml The built signed manifest XML.
     */
    static void BuildSignature(const Manifest& manifest, qcc::XmlElement* manifestElement);

    /**
     * Builds a signed manifest's "signature" element contents from a Manifest object.
     *
     * @param[in]    manifest            Reference to the input Manifest object.
     * @param[out]   signatureElement    The built signed manifest's "signature" element.
     */
    static void BuildSignatureContent(const Manifest& manifest, qcc::XmlElement* signatureElement);

    /**
     * Builds a signed manifest's "value" element from a vector of bytes object.
     *
     * @param[in]    binaryValue     Input vector of bytes.
     * @param[out]   xmlElement      XmlElement we're adding the "value" element to.
     */
    static void BuildValue(const std::vector<uint8_t>& binaryValue, qcc::XmlElement* xmlElement);
};
} /* namespace ajn */
#endif