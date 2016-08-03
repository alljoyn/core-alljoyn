#ifndef _ALLJOYN_XML_MANIFEST_TEMPLATE_CONVERTER_H
#define _ALLJOYN_XML_MANIFEST_TEMPLATE_CONVERTER_H
/**
 * @file
 * This file defines the converter for a set of PermissionPolicy::Rules in XML format
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
#error Only include XmlRulesConverter.h in C++ code.
#endif

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <alljoyn/Status.h>
#include <alljoyn/PermissionPolicy.h>
#include <map>
#include <unordered_map>
#include <string>
#include <regex>
#include "XmlRulesConverter.h"

namespace ajn {

class XmlManifestTemplateConverter : public XmlRulesConverter {
  public:

    /**
     * Initializes the static members.
     */
    static void Init();

    /**
     * Performs the static members cleanup.
     */
    static void Shutdown();

    /**
     * Return the singleton instance of the converter.
     */
    static XmlManifestTemplateConverter* GetInstance();

    /**
     * Default virtual destructor.
     */
    virtual ~XmlManifestTemplateConverter();

  protected:

    /**
     * Adds "annotation" elements to the "interface" element if required.
     *
     * @param[in]    rule                A single rule object.
     * @param[out]  interfaceElement    Reference to the built "interface" XmlElement.
     */
    virtual void BuildXmlInterfaceAnnotations(const PermissionPolicy::Rule& rule, qcc::XmlElement* interfaceElement);

  private:

    /**
     * Static converter instance. Required to enable method overwriting.
     */
    static XmlManifestTemplateConverter* s_converter;

    /*
     * Map used to extract XML member type values from the PermissionPolicy::Rule::Member::MemberType enum.
     * Cannot use unordered_map with enums as keys for some compilers.
     */
    static std::map<PermissionPolicy::Rule::SecurityLevel, std::string> s_inverseSecurityLevelMap;

    /**
     * User shouldn't be able to create their own instance of the converter.
     */
    XmlManifestTemplateConverter();
    XmlManifestTemplateConverter(const XmlManifestTemplateConverter& other);
    XmlManifestTemplateConverter& operator=(const XmlManifestTemplateConverter& other);

    /**
     * Retrieves the root element name valid for the converted XML.
     *
     * @return   Root element name.
     */
    virtual std::string GetRootElementName();

    /**
     * Retrieves a validator for the current converter.
     *
     * @return   Validator associated with this converter.
     */
    virtual XmlRulesValidator* GetValidator();

    /**
     * Returns the current converter's rule type.
     *
     * @return   Converter rule type.
     */
    virtual PermissionPolicy::Rule::RuleType GetRuleType();
};
} /* namespace ajn */
#endif
