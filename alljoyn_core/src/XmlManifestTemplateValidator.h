#ifndef _ALLJOYN_XML_MANIFEST_TEMPLATE_VALIDATOR_H
#define _ALLJOYN_XML_MANIFEST_TEMPLATE_VALIDATOR_H

/**
 * @file
 * This file defines the validator for manifests and manifest templates in XML format
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
#error Only include XmlManifestTemplateValidator.h in C++ code.
#endif

#include <qcc/platform.h>
#include <map>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <qcc/XmlElement.h>
#include "XmlRulesValidator.h"

#define SECURITY_LEVEL_ANNOTATION_NAME "org.alljoyn.Security.Level"
#define PRIVILEGED_SECURITY_LEVEL "Privileged"
#define NON_PRIVILEGED_SECURITY_LEVEL "NonPrivileged"
#define UNAUTHENTICATED_SECURITY_LEVEL "Unauthenticated"

namespace ajn {

class XmlManifestTemplateValidator : public XmlRulesValidator {
  public:

    /**
     * Mapping between the member types in string format and PermissionPolicy::Rule::Member::MemberType enum.
     */
    static std::map<std::string, PermissionPolicy::Rule::SecurityLevel> s_securityLevelMap;

    /**
     * Initializes the static members.
     */
    static void Init();

    /**
     * Performs the static members cleanup.
     */
    static void Shutdown();

    /**
     * Retrieves the singleton instance of the validator.
     */
    static XmlManifestTemplateValidator* GetInstance();

    /**
     * Default destructor.
     */
    virtual ~XmlManifestTemplateValidator()
    { }

  private:

    /**
     * Static validator instance. Required to enable method overwriting.
     */
    static XmlManifestTemplateValidator* s_validator;

    /**
     * User shouldn't be able to create their own instance of the validator.
     */
    XmlManifestTemplateValidator()
    { }

    /**
     * Retrieves the root element name valid for the converted XML.
     *
     * @return   Root element name.
     */
    virtual std::string GetRootElementName();

    /**
     * Validates that the XML "node" element's annotations are valid.
     *
     * @param[in]   annotations  XML "annotation" elements inside the "node" element.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    virtual QStatus ValidateNodeAnnotations(const std::vector<qcc::XmlElement*>& annotations);

    /**
     * Validates that the XML "interface" element's annotations are valid.
     *
     * @param[in]   annotations  XML "annotation" elements inside the "interface" element.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    virtual QStatus ValidateInterfaceAnnotations(const std::vector<qcc::XmlElement*>& annotations);

    /**
     * Validates that the D-Bus annotation is a proper org.alljoyn.Security.Level annotation.
     *
     * @param[in]   annotation  XML "annotation" element to verify.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    virtual QStatus ValidateSecurityLevelAnnotation(const qcc::XmlElement* annotation);

    /**
     * Validates that the org.alljoyn.Security.Level annotation has a proper value.
     *
     * @param[in]   annotation  XML "annotation" element to verify.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED if the XML is not following the schema.
     */
    virtual QStatus ValidateSecurityLevelAnnotationValue(const qcc::XmlElement* annotation);
};
} /* namespace ajn */
#endif
