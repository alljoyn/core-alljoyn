#ifndef _ALLJOYN_MANIFEST_XML_VALIDATOR_H
#define _ALLJOYN_MANIFEST_XML_VALIDATOR_H
/**
 * @file
 * This file defines the validator for Security 2.0 manifest in XML format
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
#error Only include XmlManifestValidator.h in C++ code.
#endif

#include "XmlValidator.h"

namespace ajn {

#define MANIFEST_ROOT_ELEMENT_CHILDREN_COUNT 4
#define THUMBPRINT_ELEMENT_CHILDREN_COUNT 2
#define SIGNATURE_ELEMENT_CHILDER_COUNT 2

#define MANIFEST_VERSION_INDEX 0
#define MANIFEST_RULES_INDEX 1
#define MANIFEST_THUMBPRINT_INDEX 2
#define MANIFEST_SIGNATURE_INDEX 3
#define OID_ELEMENT_INDEX 0
#define VALUE_ELEMENT_INDEX 1

#define VALID_MANIFEST_VERSION_NUMBER 1

#define MANIFEST_XML_ELEMENT "manifest"
#define MANIFEST_VERSION_XML_ELEMENT "version"
#define RULES_XML_ELEMENT "rules"
#define THUMBPRINT_XML_ELEMENT "thumbprint"
#define SIGNATURE_XML_ELEMENT "signature"
#define OID_XML_ELEMENT "oid"
#define VALUE_XML_ELEMENT "value"

class XmlManifestValidator : public XmlValidator {
  public:

    /**
     * Verifies the input XML follows the signed manifest XML schema
     * available under alljoyn_core/docs/manifest.xsd.
     *
     * @param[in]    manifestXml   Signed manifest in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus Validate(const qcc::XmlElement* manifestXml);

    /**
     * Default destructor.
     */
    virtual ~XmlManifestValidator()
    { }

  private:

    /**
     * Verifies the "version" XML element follows the signed manifest XML schema.
     *
     * @param[in]    version   Signed manifest version in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateManifestVersion(const qcc::XmlElement* version);

    /**
     * Verifies the contents of the "version" XML element follow the signed manifest XML schema.
     *
     * @param[in]    version   Signed manifest version in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateManifestVersionContent(const qcc::XmlElement* version);

    /**
     * Verifies the "signature" XML element follows the signed manifest XML schema.
     *
     * @param[in]    signature   Signed manifest version in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateManifestSignature(const qcc::XmlElement* signature);

    /**
     * Verifies if the "rules" XML element follows the signed manifest XML schema.
     *
     * @param[in]    rules   Rules in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateRules(const qcc::XmlElement* rules);

    /**
     * Verifies if the "thumbprint" XML element follows the signed manifest XML schema.
     *
     * @param[in]    thumbprint   Single manifest thumbprint in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateManifestThumbprint(const qcc::XmlElement* thumbprint);

    /**
     * Verifies if the "oid" XML element follows the signed manifest XML schema.
     *
     * @param[in]    oid   Signed manifest oid in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateOid(const qcc::XmlElement* xmlOid, AJ_PCSTR oid);

    /**
     * Verifies if the "oid" XML element contents follow the signed manifest XML schema.
     *
     * @param[in]    xmlOid  Signed manifest oid in XML format.
     * @param[in]    oid     Expected OID value.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateOidContent(const qcc::XmlElement* xmlOid, AJ_PCSTR oid);

    /**
     * Verifies if the "value" XML element contents follow the signed manifest XML schema.
     *
     * @param[in]    value   Signed manifest "value" element in XML format.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the XML does not follow the signed manifest XML schema.
     */
    static QStatus ValidateValueElement(const qcc::XmlElement* value);

    /**
     * Verifies if the input is a valid base64 value.
     *
     * @param[in]    value   Base64 value to validate.
     *
     * @return   #ER_OK if XML is valid.
     *           #ER_XML_MALFORMED if the value is not a valid base64 value.
     */
    static QStatus ValidateBase64Value(const qcc::String& value);
};
}/* namespace ajn */
#endif